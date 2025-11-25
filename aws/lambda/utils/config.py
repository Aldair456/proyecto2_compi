"""
Configuración y constantes para el Lambda handler
"""
from typing import Final

# Rutas del compilador
COMPILER_SOURCE: Final[str] = '/var/task/compiler'
COMPILER_PATH: Final[str] = '/tmp/compiler'

# Directorio de trabajo
WORK_DIR: Final[str] = '/tmp/compile'

# Archivos de entrada/salida
INPUT_FILE: Final[str] = 'input.c'
OUTPUT_ASM: Final[str] = 'output.asm'
OUTPUT_DEBUG: Final[str] = 'output.debug.json'

# Límites de ejecución
MAX_EMULATION_STEPS: Final[int] = 1000
COMPILATION_TIMEOUT: Final[int] = 30  # segundos

# Headers HTTP comunes
CORS_HEADERS: Final[dict] = {
    'Access-Control-Allow-Origin': '*',
    'Content-Type': 'application/json'
}

