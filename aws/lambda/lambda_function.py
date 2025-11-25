"""
Lambda handler - Entry point para AWS Lambda

Este archivo mantiene compatibilidad con serverless.yml
y delega al handler en handlers/
"""
from .handlers.lambda_function import lambda_handler

__all__ = ['lambda_handler']
