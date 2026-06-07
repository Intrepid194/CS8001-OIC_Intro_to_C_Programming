#include <inttypes.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

double add(double a, double b) {
    return a + b;
}

double subtract(double a, double b) {
    return a - b;
}

double multiply(double a, double b) {
    return a * b;
}

double divide(double a, double b) {
    if (b == 0) {
        printf("%s", "Cannot divide by ");
        return 0.0;
    }
    return a / b;
}

int main() {

    printf("%f", add(45, 56));
    printf("%s", "\n");
    printf("%f", subtract(45, 56));
    printf("%s", "\n");
    printf("%f", multiply(45, 56));
    printf("%s", "\n");
    printf("%f", divide(9, 3));
    printf("%s", "\n");
    printf("%f", divide(9, 0));
    return 0;
};