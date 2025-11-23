// Función 3: Múltiples funciones
#include <stdio.h>

int suma(int a, int b) {
    return a + b;
}

int resta(int a, int b) {
    return a - b;
}

int main() {
    int x;
    int y;
    
    x = 20;
    y = 8;
    
    printf("%d\n", suma(x, y));
    printf("%d\n", resta(x, y));
    
    return 0;
}

