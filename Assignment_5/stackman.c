#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

void print_stack(int tos_idx, int stack[65536]);

//Only the characters 0, +, <, >, ?, _, ~, (, ) are valid. Treat all others as comments—ignore them.
int main(int argc, char* argv[]) {

    struct timespec start;

    clock_gettime(CLOCK_MONOTONIC, &start);

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



    char line[1024];

    fgets(line, sizeof(line), file);

    fclose(file);

    int ops[4096];

    int op_jump_table[4096] = {0};
    int cp_jump_table[4096] = {0};

    int indx = 0;
    for (int i=0; i<sizeof(line);i++) {
        if (line[i] == '(') {
            ops[indx] = i;
            indx++;
        } else if (line[i] == ')') {
            op_jump_table[ops[indx-1]] = i;
            cp_jump_table[i] = ops[indx-1];
            indx--;
        } else if (line[i] == '\0') {
            break;
        }
    }

    if (indx > 0) {
        printf("Error: syntax error, mismatching parentheses");
        return 1;
    }


    int stack[65536];

    int tos_idx = -1;

    int num_commands = 0;

    int i = 0;
    while (i<sizeof(line)) {
        
        char command = line[i];

        if (tos_idx > 65536) {
            printf("Error: stack overflow");
            return 1;
        }

        int eol = 0;
        switch (command) {
            default:
                printf("Error: syntax error, invalid character found");
                return 1;

            case '\0':
                
                eol = 1;
                break;

            case '0':

                tos_idx++;
                stack[tos_idx] = 0;
                break;

            case '+':

                if (tos_idx == -1) {
                    printf("Error: stack underflow");
                    return 1;
                }
                stack[tos_idx]++;
                break;

            case '>':

                if (tos_idx == -1) {
                    printf("Error: stack underflow");
                    return 1;
                }

                int n = stack[tos_idx];
                

                if (n < 2 || n > tos_idx) {
                    printf("Error: stack underflow");
                    return 1;
                }

                int end = tos_idx;
                int start = end - n;

                int temp = stack[end-1];
                for (int i = end-1; i>start; i--) {
                    stack[i] = stack[i-1];
                }
                stack[start] = temp;
                tos_idx--;
                break;

            case '<': {
                if (tos_idx == -1) {
                    printf("Error: stack underflow");
                    return 1;
                }

                int n = stack[tos_idx];
            
                if (n < 2 || n > tos_idx) {
                    printf("Error: stack underflow");
                    return 1;
                }

                int end = tos_idx;               
                int start = end - n;
                
                int temp = stack[start];

                for (int i = start; i<end-1; i++) {
                    stack[i] = stack[i+1];
                }
                stack[end-1] = temp;
                tos_idx--;
                break;
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
                break;
            }
            case '_':

                if (tos_idx == -1) {
                    printf("Error: stack underflow");
                    return 1;
                }
                tos_idx--;
                break;

            case '~':

                if (tos_idx == -1) {
                    printf("Error: stack underflow");
                    return 1;
                }
                int dup = stack[tos_idx];
                tos_idx++;
                stack[tos_idx] = dup;
                break;
            case '(':
                if (stack[tos_idx] == 0) {
                    i = op_jump_table[i];
                }
                break;
            case ')':
            
                if (stack[tos_idx] != 0) {
                    i = cp_jump_table[i];
                }
                break;
        }
        if (eol == 1) {
            break;
        }
        num_commands++;
        i++;
        
    }
    printf("# instructions: %d\n", num_commands);
    printf("final stack contents: ");
    print_stack(tos_idx, stack);

    struct timespec end;
    clock_gettime(CLOCK_MONOTONIC,&end);

    double elapsed_time = (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec) /1000000000.0;

    printf("\ntime to run: %lf seconds", elapsed_time);

    return 0;

};

void print_stack(int tos_idx, int stack[65536]) {
    if (tos_idx == -1) {
        printf("empty stack");
    } else {
        for (int i=0; i<=tos_idx; i++) {
            printf("%d ", stack[i]);
        }
    }  
    return;
}