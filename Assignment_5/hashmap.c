#include <stdio.h>
#include <stdlib.h>


int main() {

    int cp_jump_table[4096];

    cp_jump_table[345] = 100;

    printf("%d", cp_jump_table[345]);
    return 0;
}