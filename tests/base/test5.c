// Test 5: For loop
#include <stdio.h>

int main() {
    int x;
    int i;
    
    x = 1;
    
    for (i = 0; i < 10; i = i + 1) {
        x = x + i;
    }
    
    printf("%d\n", x);
    
    return 0;
}

