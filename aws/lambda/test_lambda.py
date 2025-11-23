"""
Lambda handler para compilar código C y generar assembly + debug.json
Usa binario pre-compilado (NO compila en Lambda)
"""
import json
import subprocess
import os
import shutil

COMPILER_SOURCE = '/var/task/compiler'
COMPILER_PATH = '/tmp/compiler'

def ensure_compiler_ready():
    """Copia el compiler a /tmp/ si no existe y le da permisos"""
    if not os.path.exists(COMPILER_PATH):
        print(f"📦 Copying compiler from {COMPILER_SOURCE} to {COMPILER_PATH}")
        shutil.copy2(COMPILER_SOURCE, COMPILER_PATH)
        os.chmod(COMPILER_PATH, 0o755)
        print(f"✅ Compiler ready at {COMPILER_PATH}")
    else:
        print(f"✅ Compiler already in /tmp")

def lambda_handler(event, context):
    try:
        # 0. Preparar el compilador en /tmp
        ensure_compiler_ready()
        
        # 1. Parsear el body (viene como string JSON)
        if 'body' in event:
            body = json.loads(event['body'])
        else:
            body = event
        
        # 2. Obtener código fuente
        source_code = body.get('source', '')
        
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
        
        print(f"Received source code ({len(source_code)} chars)")
        
        # 3. Crear directorio temporal
        work_dir = '/tmp/compile'
        os.makedirs(work_dir, exist_ok=True)
        
        # 4. Guardar código fuente
        input_file = os.path.join(work_dir, 'input.c')
        output_asm = os.path.join(work_dir, 'output.asm')
        output_debug = os.path.join(work_dir, 'output.debug.json')
        
        with open(input_file, 'w') as f:
            f.write(source_code)
        
        print(f"Saved to {input_file}")
        
        # 5. EJECUTAR compilador desde /tmp
        cmd = [COMPILER_PATH, input_file, output_asm, '--debug']
        
        print(f"Executing: {' '.join(cmd)}")
        
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
                    'returncode': result.returncode,
                    'success': False
                })
            }
        
        print("✅ Compilation successful")
        
        # 6. Leer resultados
        if not os.path.exists(output_asm):
            return {
                'statusCode': 500,
                'headers': {
                    'Access-Control-Allow-Origin': '*',
                    'Content-Type': 'application/json'
                },
                'body': json.dumps({
                    'error': 'Assembly file not generated',
                    'success': False
                })
            }
        
        with open(output_asm, 'r') as f:
            asm_content = f.read()
        
        print(f"Assembly size: {len(asm_content)} chars")
        
        # Leer debug.json
        if os.path.exists(output_debug):
            with open(output_debug, 'r') as f:
                debug_data = json.load(f)
            print(f"Debug data loaded")
        else:
            debug_data = {
                'sourceLines': source_code.split('\n'),
                'stackFrame': [],
                'instructions': []
            }
            print("⚠️ Debug file not found, using empty structure")
        
        return {
            'statusCode': 200,
            'headers': {
                'Access-Control-Allow-Origin': '*',
                'Content-Type': 'application/json'
            },
            'body': json.dumps({
                'success': True,
                'assembly': asm_content,
                'debug': debug_data,
                'stats': {
                    'assemblySize': len(asm_content),
                    'instructionsCount': len(debug_data.get('instructions', [])),
                    'stackFrameSize': len(debug_data.get('stackFrame', [])),
                    'sourceLines': len(debug_data.get('sourceLines', []))
                }
            }, indent=2)
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