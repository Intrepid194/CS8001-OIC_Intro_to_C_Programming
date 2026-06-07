#include <stdio.h>
#include <Stdlib.h>

void mystery(char* x) {
    while (*x) {
        if (*x > 96) {
            *x -= 32;
        }
        printf("%d", x);
        x++;
    };
};

int main() {
    mystery("r");
    return 0;
}

