"""
Emulador x86-64 en Python puro para ejecutar instrucciones assembly
"""
import re
from typing import Dict, List, Tuple, Any, Optional, Union


class X86Emulator:
    """Emulador de x86-64 en Python puro"""
    
    # Registros 64-bit soportados
    REGISTERS_64BIT = ['rax', 'rbx', 'rcx', 'rdx', 'rsi', 'rdi', 'rbp', 'rsp',
                       'r8', 'r9', 'r10', 'r11', 'r12', 'r13', 'r14', 'r15']
    
    # Registros 32-bit soportados
    REGISTERS_32BIT = ['eax', 'ebx', 'ecx', 'edx']
    
    # Stack inicial
    INITIAL_STACK_POINTER = 0x7fffffffe000
    
    def __init__(self):
        """Inicializa el emulador con registros, flags y stack vacíos"""
        # Registros 64-bit
        self.regs: Dict[str, int] = {
            reg: 0 for reg in self.REGISTERS_64BIT
        }
        self.regs['rsp'] = self.INITIAL_STACK_POINTER
        
        # Flags
        self.flags: Dict[str, int] = {'ZF': 0, 'SF': 0, 'CF': 0}
        
        # Stack simulado (dirección -> valor)
        self.stack: Dict[int, int] = {}
        
        # Mapa de labels para saltos
        self.labels: Dict[str, int] = {}
    
    def get_reg(self, name: str) -> int:
        """Obtiene valor de registro (soporta 64-bit y 32-bit)"""
        if name in self.regs:
            return self.regs[name]
        
        # Registros 32-bit (32 bits bajos de r*)
        if name == 'eax':
            return self.regs['rax'] & 0xFFFFFFFF
        elif name == 'ebx':
            return self.regs['rbx'] & 0xFFFFFFFF
        elif name == 'ecx':
            return self.regs['rcx'] & 0xFFFFFFFF
        elif name == 'edx':
            return self.regs['rdx'] & 0xFFFFFFFF
        
        return 0
    
    def set_reg(self, name: str, value: int) -> None:
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
    
    def push(self, value: int) -> None:
        """Push al stack"""
        self.regs['rsp'] -= 8
        addr = self.regs['rsp']
        self.stack[addr] = value & 0xFFFFFFFFFFFFFFFF
    
    def pop(self) -> int:
        """Pop del stack"""
        addr = self.regs['rsp']
        value = self.stack.get(addr, 0)
        self.regs['rsp'] += 8
        if addr in self.stack:
            del self.stack[addr]
        return value
    
    def update_flags(self, result: int, size: int = 64) -> None:
        """Actualiza flags ZF, SF, CF basado en resultado"""
        mask = (1 << size) - 1 if size < 64 else 0xFFFFFFFFFFFFFFFF
        result_masked = result & mask
        
        # Zero Flag
        self.flags['ZF'] = 1 if result_masked == 0 else 0
        
        # Sign Flag (bit más significativo)
        sign_bit = (size - 1) if size < 64 else 63
        self.flags['SF'] = 1 if (result_masked >> sign_bit) & 1 else 0
    
    def parse_operand(self, op: str) -> Tuple[str, Any]:
        """Parsea un operando (registro, inmediato, memoria, label)"""
        op = op.strip()
        
        # Registro
        if op in self.regs or op in self.REGISTERS_32BIT:
            return ('reg', op)
        
        # Inmediato (hex o decimal)
        if op.startswith(('0x', '0X')):
            return ('imm', int(op, 16))
        try:
            return ('imm', int(op))
        except ValueError:
            pass
        
        # Memoria [reg] o [reg+offset] o [reg-offset]
        mem_match = re.match(r'\[([^\]]+)\]', op)
        if mem_match:
            expr = mem_match.group(1)
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
    
    def get_value(self, operand: Tuple[str, Any]) -> int:
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
    
    def set_value(self, operand: Tuple[str, Any], value: int) -> None:
        """Establece el valor de un operando"""
        op_type, op_data = operand
        
        if op_type == 'reg':
            self.set_reg(op_data, value)
        elif op_type == 'mem':
            base_reg, offset = op_data
            addr = self.get_reg(base_reg) + offset
            self.stack[addr] = value & 0xFFFFFFFFFFFFFFFF
    
    def execute_instruction(self, asm_line: str) -> Union[bool, str, Tuple[str, Any]]:
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
        operands = [self.parse_operand(op.strip()) 
                   for op in operands_str.split(',')] if operands_str else []
        
        try:
            return self._execute_mnemonic(mnemonic, operands)
        except Exception as e:
            print(f"Error ejecutando {asm_line}: {str(e)}")
            return True
    
    def _execute_mnemonic(self, mnemonic: str, operands: List[Tuple[str, Any]]) -> Union[bool, str, Tuple[str, Any]]:
        """Ejecuta el mnemónico específico"""
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
        
        elif mnemonic in ['idiv', 'div']:
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
        elif mnemonic in ['and', 'or', 'xor']:
            if len(operands) == 2:
                dst_val = self.get_value(operands[0])
                src_val = self.get_value(operands[1])
                if mnemonic == 'and':
                    result = dst_val & src_val
                elif mnemonic == 'or':
                    result = dst_val | src_val
                else:  # xor
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
        
        elif mnemonic in ['shl', 'sal']:
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
            print(f"Instrucción no implementada: {mnemonic}")
            return True
    
    def get_snapshot(self, instruction_data: Dict[str, Any]) -> Dict[str, Any]:
        """Genera un snapshot del estado actual del emulador"""
        # Convertir registros a formato de respuesta
        registers = {}
        all_regs = self.REGISTERS_64BIT + self.REGISTERS_32BIT
        for reg_name in all_regs:
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
        
        return {
            'instruction': instruction_data,
            'registers': registers,
            'stack': stack_list,
            'flags': {
                'ZF': self.flags['ZF'],
                'SF': self.flags['SF'],
                'CF': self.flags['CF']
            }
        }


def emulate_from_debug(debug_data: Dict[str, Any], max_steps: int = 1000) -> List[Dict[str, Any]]:
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
    
    # Mapear labels
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
            pc = _handle_call(emulator, result[1], instructions, pc)
            continue
        
        # Manejar saltos
        if isinstance(result, tuple) and result[0] == 'jump':
            pc = _handle_jump(emulator, result[1], result[2], instructions, pc)
            continue
        
        # Manejar ret
        if result == 'ret':
            pc = _handle_ret(emulator, instructions, pc)
            if pc is None:
                break
            continue
        
        pc += 1
    
    # Agregar step number a cada snapshot
    for i, snapshot in enumerate(snapshots):
        snapshot['step'] = i
    
    print(f"Emulated {len(snapshots)} execution snapshots")
    return snapshots


def _handle_call(emulator: X86Emulator, target_operand: Any, 
                 instructions: List[Dict], pc: int) -> int:
    """Maneja una instrucción call"""
    if isinstance(target_operand, tuple) and target_operand[0] == 'label':
        target_label = target_operand[1]
    else:
        target_label = str(target_operand)
    
    # Push dirección de retorno (siguiente instrucción)
    emulator.push(pc + 1)
    
    # Saltar al target
    if target_label in emulator.labels:
        return emulator.labels[target_label]
    
    # Buscar en instrucciones
    for i, inst_check in enumerate(instructions):
        asm_check = inst_check.get('assembly', '').strip().rstrip(':')
        if asm_check == target_label or asm_check == target_label + ':':
            return i
    
    return pc + 1


def _handle_jump(emulator: X86Emulator, jump_type: str, target_operand: Any,
                 instructions: List[Dict], pc: int) -> int:
    """Maneja una instrucción de salto condicional o incondicional"""
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
    
    if not should_jump or not target_operand:
        return pc + 1
    
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
        return emulator.labels[target_label]
    
    # Buscar en instrucciones
    for i, inst_check in enumerate(instructions):
        asm_check = inst_check.get('assembly', '').strip().rstrip(':')
        if asm_check == target_label or asm_check == target_label + ':':
            return i
    
    return pc + 1


def _handle_ret(emulator: X86Emulator, instructions: List[Dict], pc: int) -> Optional[int]:
    """Maneja una instrucción ret"""
    # Retornar de función: pop dirección de retorno
    if emulator.regs['rsp'] < X86Emulator.INITIAL_STACK_POINTER:
        return_addr = emulator.pop()
        if return_addr < len(instructions):
            return return_addr
        # Fin de ejecución
        return None
    
    # Stack vacío, terminar
    return None

