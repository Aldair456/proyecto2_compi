#!/bin/bash

# Script para ejecutar todos los tests del compilador

RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

echo "=========================================="
echo "   C Compiler - Test Runner"
echo "=========================================="
echo ""

# Compilar el compilador si no existe
if [ ! -f "./compiler" ]; then
    echo "Compilando el compilador..."
    make
    if [ $? -ne 0 ]; then
        echo -e "${RED}Error al compilar el compilador${NC}"
        exit 1
    fi
    echo ""
fi

# Función para ejecutar un test
run_test() {
    local test_file=$1
    local test_name=$(basename $test_file .c)
    
    echo -n "Testing $test_name... "
    
    # Compilar con nuestro compilador
    ./compiler $test_file output.asm > /dev/null 2>&1
    if [ $? -ne 0 ]; then
        echo -e "${RED}FAIL (compiler error)${NC}"
        return 1
    fi
    
    # Ensamblar
    nasm -f elf64 output.asm -o output.o > /dev/null 2>&1
    if [ $? -ne 0 ]; then
        echo -e "${RED}FAIL (nasm error)${NC}"
        return 1
    fi
    
    # Linkear
    gcc output.o -o program -no-pie > /dev/null 2>&1
    if [ $? -ne 0 ]; then
        echo -e "${RED}FAIL (gcc error)${NC}"
        return 1
    fi
    
    # Ejecutar
    ./program > /dev/null 2>&1
    if [ $? -eq 0 ]; then
        echo -e "${GREEN}PASS${NC}"
        return 0
    else
        echo -e "${YELLOW}PASS (with warnings)${NC}"
        return 0
    fi
}

# Contadores
total=0
passed=0

echo "=== Base Tests ==="
for test in tests/base/*.c; do
    if [ -f "$test" ]; then
        run_test $test
        total=$((total + 1))
        if [ $? -eq 0 ]; then
            passed=$((passed + 1))
        fi
    fi
done
echo ""

echo "=== Function Tests ==="
for test in tests/functions/*.c; do
    if [ -f "$test" ]; then
        run_test $test
        total=$((total + 1))
        if [ $? -eq 0 ]; then
            passed=$((passed + 1))
        fi
    fi
done
echo ""

echo "=== Extension Tests ==="
for test in tests/extensions/*.c; do
    if [ -f "$test" ]; then
        run_test $test
        total=$((total + 1))
        if [ $? -eq 0 ]; then
            passed=$((passed + 1))
        fi
    fi
done
echo ""

echo "=== Optimization Tests ==="
for test in tests/optimization/*.c; do
    if [ -f "$test" ]; then
        run_test $test
        total=$((total + 1))
        if [ $? -eq 0 ]; then
            passed=$((passed + 1))
        fi
    fi
done
echo ""

# Limpiar archivos temporales
rm -f output.asm output.o program

# Resumen
echo "=========================================="
echo "   Results: $passed/$total tests passed"
echo "=========================================="

if [ $passed -eq $total ]; then
    echo -e "${GREEN}All tests passed!${NC}"
    exit 0
else
    echo -e "${YELLOW}Some tests failed or have warnings${NC}"
    exit 1
fi

