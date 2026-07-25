#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>

#include "../include/lexer.h"
#include "../include/parser.h"
#include "../include/eval.h"
#include "../include/globals.h"

// bool terminate = false;
int main() {

    while (terminate == false) {

        char *command = malloc(sizeof(char)*4097);
        printf("psi>");

        if (fgets(command, 4097, stdin) == NULL) {
            printf("No meaningful input found. Exiting program...");
            free(command);
            break;
        }
        else 
        {   
            Node *token_list = NULL;
            token_list = lex(token_list, command);
            
            Node* head = token_list;
            Node* cursor = token_list;

            if (token_list == NULL) {
                free(command);
                continue;
            }
            pval* tree = parse_expression(&cursor);
            
            deleteList(head);

            pval* evaled_tree = eval_tree(tree);

            pval_print(evaled_tree);
            printf("\n");

            pval_delete(tree);
            pval_delete(evaled_tree);

            free(command);
        }

    }


    return 0;
}
