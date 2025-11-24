"""
Lambda handler para compilar código C y generar assembly + debug.json + ejecución paso a paso con emulador x86-64
"""
import json
import subprocess
import os
import shutil
import re

COMPILER_SOURCE = '/var/task/compiler'
COMPILER_PATH = '/tmp/compiler'

def ensure_compiler_ready():
    """Copia el compiler a /tmp/ SIEMPRE (Lambda puede limpiar /tmp)"""
    print(f"📦 Copying compiler from {COMPILER_SOURCE} to {COMPILER_PATH}")
    
    if not os.path.exists(COMPILER_SOURCE):
        raise Exception(f"Compiler source not found at {COMPILER_SOURCE}")
    
    shutil.copy2(COMPILER_SOURCE, COMPILER_PATH)
    os.chmod(COMPILER_PATH, 0o755)
    
    print(f"✅ Compiler ready at {COMPILER_PATH}")

# Funciones de GDB/NASM/GCC eliminadas - ahora usamos emulador Python puro

# ============================================================================
# EMULADOR x86-64
# ============================================================================

class X86Emulator:
    """Emulador de x86-64 en Python puro"""
    
    def __init__(self):
        # Registros 64-bit
        self.regs = {
            'rax': 0, 'rbx': 0, 'rcx': 0, 'rdx': 0,
            'rsi': 0, 'rdi': 0, 'rbp': 0, 'rsp': 0x7fffffffe000,
            'r8': 0, 'r9': 0, 'r10': 0, 'r11': 0,
            'r12': 0, 'r13': 0, 'r14': 0, 'r15': 0
        }
        
        # Flags
        self.flags = {'ZF': 0, 'SF': 0, 'CF': 0}
        
        # Stack simulado (dirección -> valor)
        self.stack = {}
        
        # Mapa de labels para saltos
        self.labels = {}
        
    def get_reg(self, name):
        """Obtiene valor de registro (soporta 64-bit y 32-bit)"""
        if name in self.regs:
            return self.regs[name]
        # Registros 32-bit (32 bits bajos de r*)
        if name in ['eax', 'ebx', 'ecx', 'edx']:
            base = name[1:]  # 'eax' -> 'ax' -> 'rax'
            if name == 'eax':
                return self.regs['rax'] & 0xFFFFFFFF
            elif name == 'ebx':
                return self.regs['rbx'] & 0xFFFFFFFF
            elif name == 'ecx':
                return self.regs['rcx'] & 0xFFFFFFFF
            elif name == 'edx':
                return self.regs['rdx'] & 0xFFFFFFFF
        return 0
    
    def set_reg(self, name, value):
        """Establece valor de registro (sincroniza 32-bit con 64-bit)"""
        if name in self.regs:
            self.regs[name] = value & 0xFFFFFFFFFFFFFFFF
        elif name == 'eax':
            self.regs['rax'] = (self.regs['rax'] & 0xFFFFFFFF00000000) | (value & 0xFFFFFFFF)
        elif name == 'ebx':
            self.regs['rbx'] = (self.regs['rbx'] & 0xFFFFFFFF00000000) | (value & 0xFFFFFFFF)
        elif name == 'ecx':
            self.regs['rcx'] = (self.regs['rcx'] & 0xFFFFFFFF00000000) | (value & 0xFFFFFFFF)
        elif name == 'edx':
            self.regs['rdx'] = (self.regs['rdx'] & 0xFFFFFFFF00000000) | (value & 0xFFFFFFFF)
    
    def push(self, value):
        """Push al stack"""
        self.regs['rsp'] -= 8
        addr = self.regs['rsp']
        self.stack[addr] = value & 0xFFFFFFFFFFFFFFFF
    
    def pop(self):
        """Pop del stack"""
        addr = self.regs['rsp']
        value = self.stack.get(addr, 0)
        self.regs['rsp'] += 8
        if addr in self.stack:
            del self.stack[addr]
        return value
    
    def update_flags(self, result, size=64):
        """Actualiza flags ZF, SF, CF basado en resultado"""
        mask = (1 << size) - 1 if size < 64 else 0xFFFFFFFFFFFFFFFF
        result_masked = result & mask
        
        # Zero Flag
        self.flags['ZF'] = 1 if result_masked == 0 else 0
        
        # Sign Flag (bit más significativo)
        sign_bit = (size - 1) if size < 64 else 63
        self.flags['SF'] = 1 if (result_masked >> sign_bit) & 1 else 0
    
    def parse_operand(self, op):
        """Pa un operando (registro, inmediato, memoria)"""
        op = op.strip()
        
        # Registro
        if op in self.regs or op in ['eax', 'ebx', 'ecx', 'edx']:
            return ('reg', op)
        
        # Inmediato (hex o decimal)
        if op.startswith('0x') or op.startswith('0X'):
            return ('imm', int(op, 16))
        try:
            return ('imm', int(op))
        except:
            pass
        
        # Memoria [reg] o [reg+offset] o [reg-offset]
        mem_match = re.match(r'\[([^\]]+)\]', op)
        if mem_match:
            expr = mem_match.group(1)
            # Simplificado: solo [reg] o [reg+offset]
            parts = re.split(r'[+\-]', expr)
            base_reg = parts[0].strip()
            offset = 0
            if '+' in expr:
                offset = int(parts[1].strip(), 0) if len(parts) > 1 else 0
            elif '-' in expr:
                offset = -int(parts[1].strip(), 0) if len(parts) > 1 else 0
            
            return ('mem', (base_reg, offset))
        
        # Label
        if op.startswith('.L') or op.endswith(':'):
            return ('label', op.rstrip(':'))
        
        return ('unknown', op)
    
    def get_value(self, operand):
        """Obtiene el valor de un operando"""
        op_type, op_data = operand
        
        if op_type == 'reg':
            return self.get_reg(op_data)
        elif op_type == 'imm':
            return op_data
        elif op_type == 'mem':
            base_reg, offset = op_data
            addr = self.get_reg(base_reg) + offset
            return self.stack.get(addr, 0)
        elif op_type == 'label':
            return self.labels.get(op_data, 0)
        
        return 0
    
    def set_value(self, operand, value):
        """Establece el valor de un operando"""
        op_type, op_data = operand
        
        if op_type == 'reg':
            self.set_reg(op_data, value)
        elif op_type == 'mem':
            base_reg, offset = op_data
            addr = self.get_reg(base_reg) + offset
            self.stack[addr] = value & 0xFFFFFFFFFFFFFFFF
    
    def execute_instruction(self, asm_line):
        """Ejecuta una instrucción assembly"""
        asm_line = asm_line.strip()
        
        # Ignorar labels y comentarios
        if not asm_line or asm_line.startswith(';') or asm_line.endswith(':'):
            return True
        
        # Separar mnemónico y operandos
        parts = asm_line.split(None, 1)
        if not parts:
            return True
        
        mnemonic = parts[0].lower()
        operands_str = parts[1] if len(parts) > 1 else ''
        operands = [self.parse_operand(op.strip()) for op in operands_str.split(',')] if operands_str else []
        
        try:
            # MOVIMIENTO
            if mnemonic == 'mov':
                if len(operands) == 2:
                    src_val = self.get_value(operands[1])
                    self.set_value(operands[0], src_val)
                return True
            
            elif mnemonic == 'lea':
                if len(operands) == 2 and operands[1][0] == 'mem':
                    base_reg, offset = operands[1][1]
                    addr = self.get_reg(base_reg) + offset
                    self.set_value(operands[0], addr)
                return True
            
            # ARITMÉTICA
            elif mnemonic == 'add':
                if len(operands) == 2:
                    dst_val = self.get_value(operands[0])
                    src_val = self.get_value(operands[1])
                    result = dst_val + src_val
                    self.set_value(operands[0], result)
                    self.update_flags(result)
                return True
            
            elif mnemonic == 'sub':
                if len(operands) == 2:
                    dst_val = self.get_value(operands[0])
                    src_val = self.get_value(operands[1])
                    result = dst_val - src_val
                    self.set_value(operands[0], result)
                    self.update_flags(result)
                return True
            
            elif mnemonic == 'imul':
                if len(operands) >= 2:
                    dst_val = self.get_value(operands[0])
                    src_val = self.get_value(operands[1])
                    result = dst_val * src_val
                    self.set_value(operands[0], result)
                    self.update_flags(result)
                return True
            
            elif mnemonic == 'mul':
                if len(operands) >= 1:
                    val = self.get_value(operands[0])
                    result = self.regs['rax'] * val
                    self.regs['rax'] = result & 0xFFFFFFFFFFFFFFFF
                    self.regs['rdx'] = (result >> 64) & 0xFFFFFFFFFFFFFFFF
                    self.update_flags(result)
                return True
            
            elif mnemonic == 'idiv':
                if len(operands) >= 1:
                    divisor = self.get_value(operands[0])
                    if divisor != 0:
                        dividend = self.regs['rax']
                        quotient = dividend // divisor
                        remainder = dividend % divisor
                        self.regs['rax'] = quotient & 0xFFFFFFFFFFFFFFFF
                        self.regs['rdx'] = remainder & 0xFFFFFFFFFFFFFFFF
                return True
            
            elif mnemonic == 'div':
                if len(operands) >= 1:
                    divisor = self.get_value(operands[0])
                    if divisor != 0:
                        dividend = self.regs['rax']
                        quotient = dividend // divisor
                        remainder = dividend % divisor
                        self.regs['rax'] = quotient & 0xFFFFFFFFFFFFFFFF
                        self.regs['rdx'] = remainder & 0xFFFFFFFFFFFFFFFF
                return True
            
            elif mnemonic == 'inc':
                if len(operands) == 1:
                    val = self.get_value(operands[0])
                    result = val + 1
                    self.set_value(operands[0], result)
                    self.update_flags(result)
                return True
            
            elif mnemonic == 'dec':
                if len(operands) == 1:
                    val = self.get_value(operands[0])
                    result = val - 1
                    self.set_value(operands[0], result)
                    self.update_flags(result)
                return True
            
            elif mnemonic == 'neg':
                if len(operands) == 1:
                    val = self.get_value(operands[0])
                    result = -val
                    self.set_value(operands[0], result)
                    self.update_flags(result)
                return True
            
            # LÓGICA
            elif mnemonic == 'and':
                if len(operands) == 2:
                    dst_val = self.get_value(operands[0])
                    src_val = self.get_value(operands[1])
                    result = dst_val & src_val
                    self.set_value(operands[0], result)
                    self.update_flags(result)
                return True
            
            elif mnemonic == 'or':
                if len(operands) == 2:
                    dst_val = self.get_value(operands[0])
                    src_val = self.get_value(operands[1])
                    result = dst_val | src_val
                    self.set_value(operands[0], result)
                    self.update_flags(result)
                return True
            
            elif mnemonic == 'xor':
                if len(operands) == 2:
                    dst_val = self.get_value(operands[0])
                    src_val = self.get_value(operands[1])
                    result = dst_val ^ src_val
                    self.set_value(operands[0], result)
                    self.update_flags(result)
                return True
            
            elif mnemonic == 'not':
                if len(operands) == 1:
                    val = self.get_value(operands[0])
                    result = ~val
                    self.set_value(operands[0], result)
                    self.update_flags(result)
                return True
            
            elif mnemonic == 'shl' or mnemonic == 'sal':
                if len(operands) == 2:
                    dst_val = self.get_value(operands[0])
                    shift = self.get_value(operands[1])
                    result = dst_val << shift
                    self.set_value(operands[0], result)
                    self.update_flags(result)
                return True
            
            elif mnemonic == 'shr':
                if len(operands) == 2:
                    dst_val = self.get_value(operands[0])
                    shift = self.get_value(operands[1])
                    result = dst_val >> shift
                    self.set_value(operands[0], result)
                    self.update_flags(result)
                return True
            
            # COMPARACIÓN
            elif mnemonic == 'cmp':
                if len(operands) == 2:
                    val1 = self.get_value(operands[0])
                    val2 = self.get_value(operands[1])
                    result = val1 - val2
                    self.update_flags(result)
                return True
            
            elif mnemonic == 'test':
                if len(operands) == 2:
                    val1 = self.get_value(operands[0])
                    val2 = self.get_value(operands[1])
                    result = val1 & val2
                    self.update_flags(result)
                return True
            
            # STACK
            elif mnemonic == 'push':
                if len(operands) == 1:
                    val = self.get_value(operands[0])
                    self.push(val)
                return True
            
            elif mnemonic == 'pop':
                if len(operands) == 1:
                    val = self.pop()
                    self.set_value(operands[0], val)
                return True
            
            # CONTROL
            elif mnemonic == 'nop':
                return True
            
            elif mnemonic == 'leave':
                # leave = mov rsp, rbp; pop rbp
                self.regs['rsp'] = self.regs['rbp']
                self.regs['rbp'] = self.pop()
                return True
            
            elif mnemonic == 'call':
                # call = push return_address; jmp target
                # Simplificado: solo guardamos que hay un call
                if operands:
                    return ('call', operands[0])
                return True
            
            elif mnemonic == 'ret':
                # ret = pop rip
                return 'ret'
            
            # SALTOS (se manejan en el loop principal)
            elif mnemonic in ['jmp', 'je', 'jne', 'jl', 'jg', 'jle', 'jge', 'jnz', 'jz']:
                return ('jump', mnemonic, operands[0] if operands else None)
            
            else:
                # Instrucción no implementada, continuar
                print(f"⚠️ Instrucción no implementada: {mnemonic}")
                return True
        
        except Exception as e:
            print(f"⚠️ Error ejecutando {asm_line}: {str(e)}")
            return True
    
    def get_snapshot(self, instruction_data):
        """Genera un snapshot del estado actual"""
        # Convertir registros a formato de respuesta
        registers = {}
        for reg_name in ['rax', 'rbx', 'rcx', 'rdx', 'rsi', 'rdi', 'rbp', 'rsp', 
                        'r8', 'r9', 'r10', 'r11', 'r12', 'r13', 'r14', 'r15',
                        'eax', 'ebx', 'ecx', 'edx']:
            val = self.get_reg(reg_name)
            registers[reg_name] = {
                'hex': f'0x{val:x}',
                'decimal': val if val < 2**63 else val - 2**64
            }
        
        # Convertir stack a formato de respuesta
        stack_list = []
        for addr in sorted(self.stack.keys())[:32]:  # Primeros 32 elementos
            stack_list.append({
                'address': f'0x{addr:x}',
                'value': f'0x{self.stack[addr]:x}'
            })
        
        snapshot = {
            'instruction': instruction_data,
            'registers': registers,
            'stack': stack_list,
            'flags': {
                'ZF': self.flags['ZF'],
                'SF': self.flags['SF'],
                'CF': self.flags['CF']
            }
        }
        
        return snapshot


def emulate_from_debug(debug_data, max_steps=1000):
    """
    Emula la ejecución de instrucciones desde debug.json
    
    Args:
        debug_data: Diccionario con 'instructions' array
        max_steps: Máximo número de pasos a ejecutar
    
    Returns:
        Array de snapshots con el estado después de cada instrucción
    """
    emulator = X86Emulator()
    instructions = debug_data.get('instructions', [])
    snapshots = []
    
    # Primero, mapear labels (buscar en assembly)
    for i, inst in enumerate(instructions):
        asm = inst.get('assembly', '').strip()
        if asm.endswith(':'):
            label = asm.rstrip(':')
            emulator.labels[label] = i
    
    # Snapshot inicial
    if instructions:
        first_inst = instructions[0]
        snapshots.append(emulator.get_snapshot({
            'id': -1,
            'assembly': 'INIT',
            'sourceLine': first_inst.get('sourceLine', 0),
            'line': first_inst.get('line', 0)
        }))
    
    # Ejecutar instrucciones
    pc = 0  # Program counter
    step_count = 0
    
    while pc < len(instructions) and step_count < max_steps:
        inst = instructions[pc]
        asm = inst.get('assembly', '').strip()
        
        # Ignorar labels
        if asm.endswith(':'):
            pc += 1
            continue
        
        # Ejecutar instrucción
        result = emulator.execute_instruction(asm)
        
        # Generar snapshot después de ejecutar
        snapshots.append(emulator.get_snapshot({
            'id': inst.get('id', pc),
            'assembly': asm,
            'sourceLine': inst.get('sourceLine', 0),
            'line': inst.get('line', inst.get('sourceLine', 0))
        }))
        
        step_count += 1
        
        # Manejar call
        if isinstance(result, tuple) and result[0] == 'call':
            # call: push dirección de retorno y saltar
            target_operand = result[1]
            if isinstance(target_operand, tuple) and target_operand[0] == 'label':
                target_label = target_operand[1]
            else:
                target_label = str(target_operand)
            
            # Push dirección de retorno (siguiente instrucción)
            emulator.push(pc + 1)
            
            # Saltar al target
            if target_label in emulator.labels:
                pc = emulator.labels[target_label]
            else:
                for i, inst_check in enumerate(instructions):
                    asm_check = inst_check.get('assembly', '').strip().rstrip(':')
                    if asm_check == target_label or asm_check == target_label + ':':
                        pc = i
                        break
                else:
                    pc += 1
            continue
        
        # Manejar saltos
        if isinstance(result, tuple) and result[0] == 'jump':
            jump_type, target_operand = result[1], result[2]
            should_jump = False
            
            if jump_type == 'jmp':
                should_jump = True
            elif jump_type in ['je', 'jz']:
                should_jump = emulator.flags['ZF'] == 1
            elif jump_type in ['jne', 'jnz']:
                should_jump = emulator.flags['ZF'] == 0
            elif jump_type == 'jl':
                should_jump = emulator.flags['SF'] != emulator.flags['CF']
            elif jump_type == 'jg':
                should_jump = (emulator.flags['ZF'] == 0) and (emulator.flags['SF'] == emulator.flags['CF'])
            elif jump_type == 'jle':
                should_jump = (emulator.flags['ZF'] == 1) or (emulator.flags['SF'] != emulator.flags['CF'])
            elif jump_type == 'jge':
                should_jump = emulator.flags['SF'] == emulator.flags['CF']
            
            if should_jump and target_operand:
                # Extraer el nombre del label del operando
                if isinstance(target_operand, tuple):
                    if target_operand[0] == 'label':
                        target_label = target_operand[1]
                    else:
                        target_label = str(target_operand[1])
                else:
                    target_label = str(target_operand)
                
                # Buscar label en el mapa o en instrucciones
                if target_label in emulator.labels:
                    pc = emulator.labels[target_label]
                else:
                    # Buscar en instrucciones
                    found = False
                    for i, inst_check in enumerate(instructions):
                        asm_check = inst_check.get('assembly', '').strip().rstrip(':')
                        if asm_check == target_label or asm_check == target_label + ':':
                            pc = i
                            found = True
                            break
                    if not found:
                        pc += 1
            else:
                pc += 1
        elif result == 'ret':
            # Retornar de función: pop dirección de retorno
            if emulator.regs['rsp'] < 0x7fffffffe000:  # Si hay algo en el stack
                return_addr = emulator.pop()
                if return_addr < len(instructions):
                    pc = return_addr
                else:
                    # Fin de ejecución
                    break
            else:
                # Stack vacío, terminar
                break
        else:
            pc += 1
    
    # Agregar step number a cada snapshot
    for i, snapshot in enumerate(snapshots):
        snapshot['step'] = i
    
    print(f"✅ Emulated {len(snapshots)} execution snapshots")
    return snapshots

def lambda_handler(event, context):
    try:
        print("===== EVENT DEBUG =====")
        print(json.dumps(event, indent=2))
        print("=====================")
        
        ensure_compiler_ready()
        
        print("📥 Parsing body...")
        if 'body' in event:
            body = json.loads(event['body'])
        else:
            body = event
        
        source_code = body.get('source', '')
        debug_mode = body.get('debug', False)
        optimize_mode = body.get('optimize', False)  # Flag para activar optimizaciones
        
        if not source_code:
            return {
                'statusCode': 400,
                'headers': {
                    'Access-Control-Allow-Origin': '*',
                    'Content-Type': 'application/json'
                },
                'body': json.dumps({
                    'error': 'No source code provided',
                    'success': False
                })
            }
        
        print(f"✅ Received source code ({len(source_code)} chars)")
        print(f"🔧 Debug mode: {debug_mode}, Optimize mode: {optimize_mode}")
        
        work_dir = '/tmp/compile'
        os.makedirs(work_dir, exist_ok=True)
        
        input_file = os.path.join(work_dir, 'input.c')
        output_asm = os.path.join(work_dir, 'output.asm')
        output_debug = os.path.join(work_dir, 'output.debug.json')
        
        with open(input_file, 'w') as f:
            f.write(source_code)
        
        print(f"💾 Saved to {input_file}")
        
        # Construir comando: siempre incluir --debug si está activado, y --optimize si se solicita
        cmd = [COMPILER_PATH, input_file, output_asm]
        if debug_mode:
            cmd.append('--debug')
        if optimize_mode:
            cmd.append('--optimize')
            print("⚠️ Optimization enabled - line-by-line debug may be affected")
        
        print(f"🚀 Executing: {' '.join(cmd)}")
        
        result = subprocess.run(
            cmd,
            capture_output=True,
            text=True,
            timeout=30,
            cwd=work_dir
        )
        
        if result.returncode != 0:
            return {
                'statusCode': 400,
                'headers': {
                    'Access-Control-Allow-Origin': '*',
                    'Content-Type': 'application/json'
                },
                'body': json.dumps({
                    'error': 'Compilation failed',
                    'stderr': result.stderr,
                    'stdout': result.stdout,
                    'success': False
                })
            }
        
        print("✅ Compilation successful")
        
        with open(output_asm, 'r') as f:
            asm_content = f.read()
        
        if os.path.exists(output_debug):
            with open(output_debug, 'r') as f:
                debug_data = json.load(f)
        else:
            debug_data = {
                'sourceLines': source_code.split('\n'),
                'stackFrame': [],
                'instructions': []
            }
        
        execution_snapshots = []
        if debug_mode:
            try:
                # Usar emulador x86-64 en lugar de GDB
                print("🎮 Starting x86-64 emulation...")
                execution_snapshots = emulate_from_debug(debug_data, max_steps=1000)
                print(f"✅ Execution snapshots generated: {len(execution_snapshots)} steps")
                
            except Exception as e:
                import traceback
                error_trace = traceback.format_exc()
                print(f"⚠️ Emulation failed: {str(e)}")
                print(error_trace)
                # Continuar sin snapshots si falla la emulación
        
        response_data = {
            'success': True,
            'assembly': asm_content,
            'debug': debug_data,
            'stats': {
                'assemblySize': len(asm_content),
                'instructionsCount': len(debug_data.get('instructions', [])),
                'stackFrameSize': len(debug_data.get('stackFrame', [])),
                'sourceLines': len(debug_data.get('sourceLines', []))
            }
        }
        
        if execution_snapshots:
            response_data['execution'] = execution_snapshots
            response_data['stats']['executionSteps'] = len(execution_snapshots)
        
        return {
            'statusCode': 200,
            'headers': {
                'Access-Control-Allow-Origin': '*',
                'Content-Type': 'application/json'
            },
            'body': json.dumps(response_data, indent=2)
        }
        
    except subprocess.TimeoutExpired:
        return {
            'statusCode': 408,
            'headers': {
                'Access-Control-Allow-Origin': '*',
                'Content-Type': 'application/json'
            },
            'body': json.dumps({
                'error': 'Compilation timeout (30s)',
                'success': False
            })
        }
    except Exception as e:
        import traceback
        error_trace = traceback.format_exc()
        print(f"❌ Error: {str(e)}")
        print(error_trace)
        
        return {
            'statusCode': 500,
            'headers': {
                'Access-Control-Allow-Origin': '*',
                'Content-Type': 'application/json'
            },
            'body': json.dumps({
                'error': str(e),
                'trace': error_trace,
                'success': False
            })
        }