#!/bin/bash
# Script para desplegar el Lambda usando Serverless Framework

set -e

echo "🚀 Desplegando Lambda function..."

# Verificar que serverless esté instalado
if ! command -v serverless &> /dev/null; then
    echo "❌ Serverless Framework no está instalado"
    echo "   Instalar con: npm install -g serverless"
    exit 1
fi

# Ir al directorio aws
cd "$(dirname "$0")"

# Empaquetar primero
echo "📦 Empaquetando..."
./package.sh

# Desplegar
echo "🚀 Desplegando a AWS..."
serverless deploy

echo ""
echo "✅ Deployment completo!"
echo ""
echo "📋 Para ver los logs:"
echo "   serverless logs -f compile -t"
echo ""
echo "📋 Para eliminar:"
echo "   serverless remove"

