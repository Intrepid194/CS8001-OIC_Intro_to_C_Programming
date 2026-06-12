#include <stdio.h>
#include <stdlib.h>
#include <string.h>


//Only the characters 0, +, <, >, ?, _, ~, (, ) are valid. Treat all others as comments—ignore them.
int main(int argc, char* argv[]) {

    if (argc > 2) {
        printf("Error: Too many input arguments.");
        return 1;
    }

    if (argv[1] == "") {
        printf("Error: No filename was provided.");
        return 1;
    }

    const char* idx = strstr(argv[1], ".sm");

    if (idx == NULL) {
        printf("Error: File extension is not .sm, unable to open file.");
        return 1;
    }

    FILE* file;
    char* filename = argv[1];

    file = fopen(filename, "r");  

    if (file == NULL) {
        printf("Error: File does not exist or unable to open file.");
        return 1;
    }

    int stack[65536];

    int tos_idx = -1;

    char line[1024];

    fgets(line, sizeof(line), file);

    fclose(file);

    int line_length = 0;

    for (int i=0; i<sizeof(line); i++) {
        
        char command = line[i];

        switch (command) {
            case '0':

                tos_idx++;
                stack[tos_idx] = 0;
                continue;

            case '+':

                if (tos_idx == -1) {
                    printf("Error: stack underflow");
                    return 1;
                }
                stack[tos_idx]++;
                continue;

            case '<':

                if (tos_idx == 3) {
                    printf("Error: stack underflow");
                    return 1;
                }
                continue;

            case '>': {
                if (tos_idx == -1) {
                    printf("Error: stack underflow");
                    return 1;
                }

                int n = stack[tos_idx];

                if (n < 2 || n > tos_idx) {
                    printf("Error: stack underflow");
                    return 1;
                }

                int mid;
                int end = tos_idx-1;

                if (n % 2 == 0) {
                    mid = n /2;
                } else if (n % 2 > 0) {
                    mid = (n-1)/2;
                }
                
                for (int i=0; i<mid; i++) {
                    int temp = stack;
                }
                
                continue;
            }  
            case '?': {

                if (tos_idx == -1) {
                    printf("Error: stack underflow");
                    return 1;
                }
                if (stack[tos_idx] == 0) {
                    tos_idx++;
                    stack[tos_idx] = 0;
                } else if (stack[tos_idx] > 0) {
                    stack[tos_idx]--;
                    tos_idx++;
                    stack[tos_idx] = 1;
                }
                continue;
            }
            case '_':

                if (tos_idx == -1) {
                    printf("Error: stack underflow");
                    return 1;
                }
                tos_idx--;
                continue;

            case '~':

                if (tos_idx == -1) {
                    printf("Error: stack underflow");
                    return 1;
                }
                int dup = stack[tos_idx];
                tos_idx++;
                stack[tos_idx] = dup;
                continue;

            case '(':

                continue;

            case ')':

            continue;

        }
        if (line[i] == '\0') {
            break;
        }
    }
    
    // if (tos_idx == -1) {
    //     printf("empty stack");
    // } else {
    //     for (int i=0; i<=tos_idx; i++) {
    //         printf("%d ", stack[i]);
    //     }
    // }    

    return 0;

}