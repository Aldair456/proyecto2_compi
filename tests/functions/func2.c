// Función 2: Múltiples parámetros
#include <stdio.h>

int multiplicar(int a, int b) {
    int resultado;
    resultado = a * b;
    return resultado;
}

int main() {
    int x;
    int y;
    
    x = 7;
    y = 6;
    
    printf("%d\n", multiplicar(x, y));
    
    return 0;
}

