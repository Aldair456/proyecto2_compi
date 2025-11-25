"""
Utilidades para construir respuestas HTTP del Lambda
"""
import json
from typing import Dict, Any, Optional
from .config import CORS_HEADERS


class ResponseBuilder:
    """Builder para construir respuestas HTTP consistentes"""
    
    @staticmethod
    def success(data: Dict[str, Any], status_code: int = 200) -> Dict[str, Any]:
        """Construye una respuesta exitosa"""
        return {
            'statusCode': status_code,
            'headers': CORS_HEADERS,
            'body': json.dumps(data, indent=2)
        }
    
    @staticmethod
    def error(message: str, status_code: int = 400, 
             details: Optional[Dict[str, Any]] = None) -> Dict[str, Any]:
        """Construye una respuesta de error"""
        error_data = {
            'error': message,
            'success': False
        }
        
        if details:
            error_data.update(details)
        
        return {
            'statusCode': status_code,
            'headers': CORS_HEADERS,
            'body': json.dumps(error_data)
        }
    
    @staticmethod
    def compilation_failed(stderr: str, stdout: str) -> Dict[str, Any]:
        """Construye una respuesta para fallo de compilación"""
        return ResponseBuilder.error(
            'Compilation failed',
            status_code=400,
            details={
                'stderr': stderr,
                'stdout': stdout
            }
        )
    
    @staticmethod
    def timeout_error(timeout_seconds: int) -> Dict[str, Any]:
        """Construye una respuesta para timeout"""
        return ResponseBuilder.error(
            f'Compilation timeout ({timeout_seconds}s)',
            status_code=408
        )
    
    @staticmethod
    def internal_error(error: Exception, trace: Optional[str] = None) -> Dict[str, Any]:
        """Construye una respuesta para error interno"""
        error_data = {
            'error': str(error),
            'success': False
        }
        
        if trace:
            error_data['trace'] = trace
        
        return {
            'statusCode': 500,
            'headers': CORS_HEADERS,
            'body': json.dumps(error_data)
        }

