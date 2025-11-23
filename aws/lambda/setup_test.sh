#!/bin/bash
# Script para configurar el entorno de prueba del Lambda

echo "🔧 Setting up Lambda test environment..."

# Ir al directorio raíz del proyecto
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/../.." && pwd)"

echo "📁 Project root: $PROJECT_ROOT"
echo "📁 Lambda dir: $SCRIPT_DIR"

# Compilar el compilador si no existe
if [ ! -f "$PROJECT_ROOT/compiler" ] && [ ! -f "$PROJECT_ROOT/compiler.exe" ]; then
    echo "🔨 Compiling compiler..."
    cd "$PROJECT_ROOT"
    make
    if [ $? -ne 0 ]; then
        echo "❌ Failed to compile compiler"
        exit 1
    fi
fi

# Copiar compilador al directorio lambda
if [ -f "$PROJECT_ROOT/compiler" ]; then
    echo "📋 Copying compiler to lambda directory..."
    cp "$PROJECT_ROOT/compiler" "$SCRIPT_DIR/compiler"
    chmod +x "$SCRIPT_DIR/compiler"
elif [ -f "$PROJECT_ROOT/compiler.exe" ]; then
    echo "📋 Copying compiler.exe to lambda directory..."
    cp "$PROJECT_ROOT/compiler.exe" "$SCRIPT_DIR/compiler"
    chmod +x "$SCRIPT_DIR/compiler"
else
    echo "❌ Compiler not found"
    exit 1
fi

echo "✅ Setup complete!"
echo ""
echo "🚀 To test the Lambda handler:"
echo "   cd aws/lambda"
echo "   python test_lambda.py test_event_simple.json"

