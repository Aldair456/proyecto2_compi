// Optimización 4: Algebraic simplification
// x * 2 puede optimizarse a x << 1 (shift)
#include <stdio.h>

int main() {
    int x;
    int y;
    
    x = 10;
    y = x * 2;  // Puede optimizarse con shift
    
    printf("%d\n", y);
    
    return 0;
}

