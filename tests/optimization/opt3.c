// Optimización 3: Constant propagation
#include <stdio.h>

int main() {
    int x;
    int y;
    int z;
    
    x = 5;
    y = x;  // y = 5
    z = y + 3;  // z = 8
    
    printf("%d\n", z);
    
    return 0;
}

