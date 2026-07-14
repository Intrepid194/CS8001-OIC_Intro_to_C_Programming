#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>

#include "lexer.h"


//Parser Prototypes

typedef enum {
    PVAL_NUMBER,
    PVAL_BOOL,
    PVAL_SYMBOL,
    PVAL_LIST,
    PVAL_FUNCTION,
    PVAL_ERROR
} pval_tag;

typedef struct pval pval;

typedef struct pval {
    pval_tag pval_type;
    union {
        int64_t pval_number;
        bool pval_bool;
        char *pval_symbol;
        struct list *pval_list;
        struct function *pval_function;
        struct pval *pval_error;
    };
} pval;

typedef struct list_child {
    pval *atom;

    struct list_child *next;
} list_child;

typedef struct list {
    int list_count;

    list_child *atoms_head;
} list;

typedef struct function {
    int r;
} function;

pval* pval_number(int64_t n);

pval* pval_bool(bool b);

pval* pval_error(pval* x);

pval* pval_symbol(char *symbol);

pval* empty_list();

void pval_add(pval* list, pval* elem);

void pval_print(pval* pv);

pval* pval_copy(pval* pv);

void pval_delete(pval* pv);

void parse(Node *token_list_head);

int main() {

    while (1) {
        Node *token_list = NULL;

        char *command = malloc(sizeof(char)*4097);
        printf("psi>");

        if (fgets(command, 4097, stdin) == NULL) {
            printf("No meaningful input found. Exiting program...");
            free(command);
            break;
        }
        else 
        {
            token_list = lex(token_list, command);

        }
        parse(token_list);
        // printList(token_list);
        deleteList(token_list);
    }


    return 0;
}

//Parser function def
//pval constructor functions
pval* pval_number(int64_t n) 
{
    pval *pval_num = (pval *)malloc(sizeof(pval));

    pval_num->pval_type = PVAL_NUMBER;
    pval_num->pval_number = n;

    return pval_num;
};

pval* pval_bool(bool b) 
{
    pval* pval_bl = (pval *)malloc(sizeof(pval));

    pval_bl->pval_type = PVAL_BOOL;
    pval_bl->pval_bool = b;
    
    return pval_bl;
};

pval* pval_symbol(char *symbol)
{
    pval* pval_sym = (pval *)malloc(sizeof(pval));

    pval_sym->pval_type = PVAL_SYMBOL;

    //duplicate the symbol tokens in the tokenized list so the token list can be freed after it is parsed.
    //need to free this symbol when defining pval_delete().
    pval_sym->pval_symbol = strdup(symbol);

    return pval_sym;
};

pval* pval_function() {};

pval* pval_error(pval* x) {};

//makes empty list to add pvals to later by calling pval_add().
pval* empty_list()
{
    pval *em_list = (pval *)malloc(sizeof(pval));

    em_list->pval_type = PVAL_LIST;

    list *new_list = (list *)malloc(sizeof(list));
    new_list->list_count = 0;
    new_list->atoms_head = NULL;

    em_list->pval_list = new_list;

    return em_list;
};

//adds a pval element to the list passed into it.
void pval_add(pval* list, pval* elem)
{
    list_child *new_child = (list_child *)malloc(sizeof(list_child));
    new_child->atom = elem;
    new_child->next = NULL;

    if (list->pval_list->atoms_head == NULL) {

        list->pval_list->atoms_head = new_child;
        list->pval_list->list_count++;
        return;
    }

    list_child *temp = list->pval_list->atoms_head;

    while (temp->next != NULL) {
        temp = temp->next;
    }

    temp->next = new_child;
    list->pval_list->list_count++;
    return;
};

void pval_print(pval* pv) {};

pval* pval_copy(pval* pv) {};

void pval_delete(pval* pv) {};

void parse(Node *token_list_head) 
{   
    
    Node* current = token_list_head;

    pval* new_list = NULL;
    if (current->token == '(')
    {
        new_list = empty_list();
    }
    else if (current->token[0] == '#')
    {   
        bool temp = false;

        if (strcmp(current->token, "#t") == 0) {
            temp = true;
        }
        else if (strcmp(current->token, "#f") == 0)
        {
            temp = false;
        }
        pval* b = pval_bool(temp);
    }

    
    // while (current != NULL) {
    //     if (current->token == '(') 
    //     {
    //         pval* new_list = empty_list();
    //     }
    //     else if (current->token == ')') 
    //     {
    //         return;
    //     } 
    //     else if (current->token[0] =='#') //parse a boolean (#t or #f)
    //     {
    //         pval* b = pval_bool(current->token);
    //     }
    //     else if (((current->token[0] == '-' || current->token[0] == '+') && (current->token[1] >= 48 && current->token[1] <= 58)) || (current->token[1] >= 48 && current->token[1] <= 58)) //parse a number
    //     {
    //         pval* number = pval_number(current->token);
    //     }
    //     else
    //     {
    //         pval* symbol = pval_symbol(current->token);
    //     }
    //     printf("%s ", current->token);
    //     current = current->next;
    // }

};

