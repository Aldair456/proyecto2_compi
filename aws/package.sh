#!/bin/bash
# Script para empaquetar el Lambda con todos los archivos necesarios

set -e

echo "📦 Empaquetando Lambda function..."

# Directorios
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"
LAMBDA_DIR="$SCRIPT_DIR/lambda"
PACKAGE_DIR="$SCRIPT_DIR/package"
ZIP_FILE="$SCRIPT_DIR/lambda_package.zip"

# Limpiar directorio de empaquetado
echo "🧹 Limpiando directorio de empaquetado..."
rm -rf "$PACKAGE_DIR"
mkdir -p "$PACKAGE_DIR"

# Copiar lambda function
echo "📋 Copiando lambda function..."
cp "$LAMBDA_DIR/lambda_function.py" "$PACKAGE_DIR/"

# Copiar archivos fuente del compilador
echo "📋 Copiando archivos fuente del compilador..."

# Crear estructura de directorios
mkdir -p "$PACKAGE_DIR/scanner"
mkdir -p "$PACKAGE_DIR/parser"
mkdir -p "$PACKAGE_DIR/visitors"

# Copiar main.cpp
cp "$PROJECT_ROOT/main.cpp" "$PACKAGE_DIR/"

# Copiar scanner
cp "$PROJECT_ROOT/scanner/"*.cpp "$PACKAGE_DIR/scanner/"
cp "$PROJECT_ROOT/scanner/"*.h "$PACKAGE_DIR/scanner/"

# Copiar parser
cp "$PROJECT_ROOT/parser/"*.cpp "$PACKAGE_DIR/parser/"
cp "$PROJECT_ROOT/parser/"*.h "$PACKAGE_DIR/parser/"

# Copiar visitors
cp "$PROJECT_ROOT/visitors/"*.cpp "$PACKAGE_DIR/visitors/"
cp "$PROJECT_ROOT/visitors/"*.h "$PACKAGE_DIR/visitors/"

# Crear ZIP
echo "📦 Creando ZIP..."
cd "$PACKAGE_DIR"
zip -r "$ZIP_FILE" . -q

# Mostrar tamaño
SIZE=$(du -h "$ZIP_FILE" | cut -f1)
echo "✅ Package creado: $ZIP_FILE ($SIZE)"

# Mostrar contenido
echo ""
echo "📄 Contenido del package:"
unzip -l "$ZIP_FILE" | head -20

echo ""
echo "🚀 Para desplegar:"
echo "   aws lambda update-function-code \\"
echo "     --function-name compiler-debugger-dev-compile \\"
echo "     --zip-file fileb://$ZIP_FILE"

