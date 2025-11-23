// Optimización 2: Dead code elimination
// El compilador puede eliminar código que nunca se ejecuta
#include <stdio.h>

int main() {
    int x;
    int y;
    
    x = 10;
    
    if (0) {
        // Este código nunca se ejecuta (dead code)
        y = 100;
    }
    
    printf("%d\n", x);
    
    return 0;
}

