#include <stdio.h>
#define RESET "\x1b[0m"
#define BLUE "\x1b[30m"
#define RED "\x1b[31m"

int main() {
    
    int n, i;

    printf(RED"Enter a number: "RESET);
    scanf("%d", &n);

    
    for (int i = 1; i <= 10; i++) {
        printf(BLUE"%d * %d = %d\n"RESET, n, i, n * i);
    }

    
    return 0;
}