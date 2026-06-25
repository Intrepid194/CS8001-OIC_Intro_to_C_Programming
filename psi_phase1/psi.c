#include <stdio.h>
#include <stdlib.h>
#include <string.h>


typedef struct {
    char token_type[13];
    char character[1];
    struct Node *next;
} Node;

int main() {

    Node *head = NULL;

    while (1) {
        char *command = malloc(sizeof(char)*4097);
        printf("psi>");

        fgets(command, 4097, stdin);


        printf("%s", command);

        for (int i=0; command[i] != '\0'; i++) {
            switch (command[i]) {
                case '(':
                    
                    break;
                case ')':
                    break;
            }
        }

        // char *lexer = malloc(sizeof(char)*4097);
        // char* token = strtok(command, "");

        // while (token != NULL) {
        //     printf("%s\n", token);
        //     token= strtok(NULL, "");
        // }

        if (command[0] == 'q') {
            free(command);
            break;
        }
        free(command);
    }


    return 0;
}


char parse(char command[4096]) {


    return 1;
}
