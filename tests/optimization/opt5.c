// Optimización 5: Loop unrolling
// El compilador puede "desenrollar" loops pequeños
#include <stdio.h>

int main() {
    int sum;
    int i;
    
    sum = 0;
    
    for (i = 0; i < 4; i = i + 1) {
        sum = sum + i;
    }
    
    printf("%d\n", sum);
    
    return 0;
}

