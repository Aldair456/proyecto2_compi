// Función 1: Función simple que retorna
#include <stdio.h>

int suma(int a, int b) {
    return a + b;
}

int main() {
    int x;
    int y;
    int result;
    
    x = 1;
    y = 20;
    result = suma(x, y);
    
    printf("%d\n", result);
    
    return 0;
}

