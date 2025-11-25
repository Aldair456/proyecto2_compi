"""
Lambda handler principal - Entry point para AWS Lambda

Este handler es simple y delega toda la lógica al servicio de compilación.
"""
import json
from typing import Dict, Any

from ..services.compilation_service import CompilationService


# Instancia global del servicio (reutilizada entre invocaciones)
_service = CompilationService()


def lambda_handler(event: Dict[str, Any], context: Any) -> Dict[str, Any]:
    """
    Handler principal de Lambda
    
    Args:
        event: Evento de Lambda con el request
        context: Contexto de Lambda
        
    Returns:
        Respuesta HTTP formateada
    """
    print("===== EVENT DEBUG =====")
    print(json.dumps(event, indent=2))
    print("=====================")
    
    return _service.process_request(event)

