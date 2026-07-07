#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>



typedef enum {
    PVAL_NUMBER,
    PVAL_BOOL,
    PVAL_SYMBOL,
    PVAL_LIST,
    PVAL_FUNCTION,
    PVAL_ERROR
} pval_tag;

typedef struct pval {
    pval_tag pval_type;
    union {
        int64_t pval_number;
        bool pval_bool;
        char *pval_symbol;
        char *pval_list;
        char *pval_function;
        char *pval_error;
    };
} pval;

typedef struct Node {
    char *token;
    struct Node *next;
} Node;

void printList(Node *node);

char *createToken(char command);

Node *createNode(char *token);

void append(Node **head, char *token);

int main() {

    Node *head = NULL;

    while (1) {
        char *command = malloc(sizeof(char)*4097);
        printf("psi>");

        fgets(command, 4097, stdin);

        for (int i=0; command[i] != '\0'; i++) {

            switch (command[i]) {
                case '(':

                    append(&head, createToken(command[i]));
                    break;
                case ')':

                    append(&head, createToken(command[i]));
                    break;

                case '+':
                    append(&head, createToken(command[i]));
                    break;

                case '-':
                    if (i+1 < strlen(command)) {
                        if (command[i+1] == ' ') {
                            append(&head, createToken(command[i]));
                        } else {
                            char *sub = (char *)calloc(4097, sizeof(char));
                            int sub_idx = 0;

                            for (int j = i; (command[j] != '\0') && (command[j] != ' '); j++) {
                                if (command[j] >= 48 && command[j] <= 57) {
                                    sub[sub_idx] = command[j];
                                    sub_idx++;
                                }
                                i = j;
                            } 

                            append(&head, sub);
                        }
                    }

                    break;

                case '/':
                    append(&head, createToken(command[i]));
                    break;

                case ' ':

                    break;

                case '0' ... '9':
                    if (i+1 < strlen(command)) {
                        if (command[i+1] == ' ' || command[i+1] == ')') {
                            append(&head, createToken(command[i]));
                        } else {

                            char *temp = (char *)calloc(4097, sizeof(char));
                            int temp_idx = 0;

                            for (int j = i; (command[j] != ')') && (command[j] != '\0') && (command[j] != ' '); j++) {
                                if (command[j] >= 48 && command[j] <= 57) {
                                    temp[temp_idx] = command[j];
                                    temp_idx++;
                                }
                                i = j;
                            } 

                            append(&head, temp);
                        }
                    }

                    break;

            }
        }


        if (command[0] == 'q') {
            free(command);
            break;
        }
        free(command);
    }

    printList(head);
    return 0;
}

char *createToken(char command) {

    char *token = malloc(sizeof(char)*2);
    token[0] = command;
    token[1] = '\0';

    return token;
}

Node *createNode(char *token) { 

    Node *newNode = (Node *)malloc(sizeof(Node));

    newNode->token = token;
    newNode->next = NULL;

    return newNode;
}

void append(Node** head, char *token) {

    Node *new_node = createNode(token);

    Node *last = *head;

    if (*head == NULL) {

        *head = new_node;
        return;
    }

    while (last->next != NULL) {
        last = last->next;
    }

    last->next = new_node;
    return;
}

void printList(Node *node)
{
  while (node != NULL)
  {
     printf(" %s ", node->token);
     node = node->next;
  }
}