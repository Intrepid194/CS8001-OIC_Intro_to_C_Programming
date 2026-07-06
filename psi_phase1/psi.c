#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct Node {
    char *token;
    struct Node *next;
};

int main() {

    struct Node *head = NULL;

    while (1) {
        char *command = malloc(sizeof(char)*4097);
        printf("psi>");

        fgets(command, 4097, stdin);

        // printf("%s", command);
        char *temp = malloc(sizeof(char)*4097);
        int temp_idx = 0;

        for (int i=0; command[i] != '\0'; i++) {

            switch (command[i]) {
                case '(':
                    struct Node *newNode = malloc(sizeof(struct Node));
                    
                    
                    newNode->token = &command[i];
                    newNode->next = NULL;

                    struct Node *last = head;

                    if (head == NULL) {
                        head = newNode;
                        continue;
                    }
                    while (last->next != NULL) {
                        last = last->next;
                    }

                    last->next = newNode;

                    break;
                case ')':
                    break;

                case '+':
                    break;

                case '-':
                    break;

                case '/':
                    break;

                case ' ':
                    temp[0] = '\0';
                    temp_idx = 0;
                    continue;

                case '0' ... '9': case'a' ... 'z': case 'A' ... 'Z':

                    temp[temp_idx] = command[i];
                    temp_idx++;

                    break;
            }
        }

        // char *lexer = malloc(sizeof(char)*4097);
        // char* token = strtok(command, " ");

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

// Node *createNode(char temp[], int n) { 

//     Node *newNode = (Node *)malloc(sizeof(Node));

//     newNode->token = (char *)(malloc(sizeof(char) * n));
//     newNode->next = NULL;

//     return newNode;
// }

// void append(Node** head, char *temp, int n) {

//     Node *new_node = createNode(temp, n);

//     Node *last = *head;

//     if (*head == NULL) {

//         *head = new_node;
//         return;
//     }

//     while (last->next != NULL) {
//         last = last->next;
//     }

//     last->next = new_node;
//     return;
// }

char parse(char command[4096]) {


    return 1;
}
