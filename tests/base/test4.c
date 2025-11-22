// Test 4: While loop
#include <stdio.h>

int main() {
    int x;
    int i;
    
    x = 0;
    i = 0;
    
    while (i < 5) {
        x = x + i;
        i = i + 1;
    }
    
    printf("%d\n", x);
    
    return 0;
}

