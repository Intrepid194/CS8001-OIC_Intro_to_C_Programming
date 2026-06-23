#include <inttypes.h>
#include <stdio.h>

int main () {
    uint8_t a = 37;
    uint8_t b = 38;
    int c = (int) (uint8_t) (a - b);
    int d = (int) (a - b);

    if (c == d) {
        printf("equal\n");
    } else {
        printf("not equal\n");
    }
    printf("%d\n", c);
    printf("%d", d);
    return 0;
}
