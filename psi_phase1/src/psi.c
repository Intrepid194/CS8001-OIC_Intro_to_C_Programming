#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>

#include "../include/lexer.h"
#include "../include/parser.h"
#include "../include/eval.h"
#include "../include/globals.h"
#include "../include/pvals.h"

int main(int argc, char * argv[]) {
    //create global environment
    environment *global_env = create_env();

    //create and bindings to global env
    //phase 1 built ins
    env_add_binding(global_env, create_binding("+", pval_func(add, "+")));
    env_add_binding(global_env, create_binding("-", pval_func(subtract, "-")));
    env_add_binding(global_env, create_binding("*", pval_func(multiply, "*")));
    env_add_binding(global_env, create_binding("/", pval_func(divide, "/")));
    env_add_binding(global_env, create_binding("quit", pval_func(exit_prg, "quit")));
    env_add_binding(global_env, create_binding("=", pval_func(equals, "=")));

    //phase 2 builtins
    env_add_binding(global_env, create_binding("head", pval_func(head, "head")));
    env_add_binding(global_env, create_binding("tail", pval_func(tail, "tail")));
    env_add_binding(global_env, create_binding("type", pval_func(type, "type")));

    env_add_binding(global_env, create_binding(">", pval_func(gt, ">")));
    env_add_binding(global_env, create_binding("<", pval_func(lt, "<")));
    env_add_binding(global_env, create_binding(">=", pval_func(gte, ">=")));
    env_add_binding(global_env, create_binding("<=", pval_func(lte, "<=")));
    env_add_binding(global_env, create_binding("!=", pval_func(not_eq, "!=")));
    env_add_binding(global_env, create_binding("not", pval_func(not, "not")));
    env_add_binding(global_env, create_binding("_", empty_list()));
    env_add_binding(global_env, create_binding("cons", pval_func(cons, "cons")));

    env_add_binding(global_env, create_binding("ord", pval_func(ord, "ord")));
    env_add_binding(global_env, create_binding("chr", pval_func(str_to_chr, "chr")));
    env_add_binding(global_env, create_binding("output", pval_func(output, "output")));
    env_add_binding(global_env, create_binding("input", pval_func(input, "input")));

    //phase 3 bindings
    env_add_binding(global_env, create_binding("%", pval_func(modulo, "%")));
    env_add_binding(global_env, create_binding("cell", pval_func(create_cell, "cell")));
    env_add_binding(global_env, create_binding("!", pval_func(deref_cell, "!")));
    env_add_binding(global_env, create_binding(":=", pval_func(write_cell, ":="))); 
    env_add_binding(global_env, create_binding("print", pval_func(print, "print")));
    env_add_binding(global_env, create_binding("str", pval_func(str, "str")));
    env_add_binding(global_env, create_binding("list", pval_func(make_list, "list")));
    env_add_binding(global_env, create_binding("error", pval_func(error, "error")));
    env_add_binding(global_env, create_binding("read", pval_func(read, "read")));
    env_add_binding(global_env, create_binding("get-file", pval_func(get_file, "get-file")));
    env_add_binding(global_env, create_binding("put-file", pval_func(put_file, "put-file")));
    env_add_binding(global_env, create_binding("new-symbol", pval_func(new_symbol, "new-symbol")));


    int flag = 0;
    while (terminate == false) {

        if (argc > 1)
        {
            pval* filename = pval_string(argv[1], strlen(argv[1]));

            pval* lst = empty_list();
            pval_add(lst, pval_symbol("load"));
            pval_add(lst, filename);

            pval* result = eval_tree(global_env, lst);

            if (result->pval_type == PVAL_ERROR)
            {
                pval_delete(lst);
                pval_print(result);
                pval_delete(result);
                terminate = true;
                flag = 1;
                break;
            }
            pval_delete(lst);
            pval_delete(result);
            terminate = true;
        }
        else
        {
            char *command = (char *)calloc(4097, sizeof(char));
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

                // printList(token_list);

                Node* head = token_list;
                
                Node* cursor = token_list;
            
                // printList(token_list);
                if (token_list == NULL) {
                    free(command);
                    continue;
                }
                pval* tree = parse_expression(&cursor);
                deleteList(head);

                //checks if the entire line was parsed
                if ((cursor != NULL ) && (tree->pval_type != PVAL_ERROR))
                {
                    char *err_msg = "error: incomplete parse  cursor not NULL.";
                    pval *temp = tree;
                    tree = pval_error(pval_symbol(err_msg));
                    pval_delete(temp);
                }
                
                pval* evaled_tree = eval_tree(global_env, tree);


                // pval_print(tree);
                pval_print(evaled_tree);
                printf("\n");

                if (evaled_tree->pval_type != PVAL_ERROR)
                {
                    env_add_binding(global_env, create_binding("_", pval_copy(evaled_tree)));
                }
                pval_delete(tree);
                pval_delete(evaled_tree);

                free(command);
            }
            
        }
    }
    delete_env(global_env);
    return flag;
}
