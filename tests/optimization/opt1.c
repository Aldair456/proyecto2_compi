// Optimización 1: Constant folding
// El compilador puede optimizar 2 + 3 a 5 en tiempo de compilación
#include <stdio.h>

int main() {
    int x;
    
    x = 2 + 3;  // Puede optimizarse a x = 5
    
    printf("%d\n", x);
    
    return 0;
}

