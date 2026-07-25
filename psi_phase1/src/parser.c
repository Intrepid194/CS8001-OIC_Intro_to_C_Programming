#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>

#include "../include/parser.h"

//Parser function def
//pval constructor functions

pval* pval_func(pval_function pval_fn, char* func_name)
{
    pv_func *fn_struct = (pv_func *)malloc(sizeof(pv_func));

    fn_struct->func_name = strdup(func_name);
    fn_struct->pval_fn_def = pval_fn;

    pval* fn = (pval *)malloc(sizeof(pval));

    fn->pval_type = PVAL_FUNCTION;
    fn->pval_func = fn_struct;
   
    return fn;
}

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


pval* pval_error(pval* x)
{
    pval *error = (pval *)malloc(sizeof(pval));
    error->pval_type = PVAL_ERROR;
    error->pval_error = x;

    return error;
};

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

void pval_print(pval* pv)
{
    if (pv->pval_type == PVAL_BOOL)
    {
        if (pv->pval_bool == true)
        {
            printf("#t ");
        }
        else
        {
            printf("#f ");
        }
    }
    else if (pv->pval_type == PVAL_NUMBER)
    {
        printf("%lld ", (long long)pv->pval_number);
    }
    else if (pv->pval_type == PVAL_SYMBOL)
    {
        printf("%s ", pv->pval_symbol);
    }
    else if (pv->pval_type == PVAL_ERROR)
    {
        pval_print(pv->pval_error);
    }
    else if (pv->pval_type == PVAL_FUNCTION)
    {
        printf("%s ", pv->pval_func->func_name);
    }
    else if (pv->pval_type == PVAL_LIST)
    {
        printf("(");
        list_child *temp = pv->pval_list->atoms_head;
        while (temp != NULL) 
        {
            pval_print(temp->atom);
            temp = temp->next;
        }
        printf(")");
    }
};



void pval_delete(pval* pv)
{
    if (pv->pval_type == PVAL_BOOL)
    {
        free(pv);
    }
    else if (pv->pval_type == PVAL_NUMBER)
    {
        free(pv);
    }
    else if (pv->pval_type == PVAL_SYMBOL)
    {
        char* temp_symbol = pv->pval_symbol;
        free(temp_symbol);

        free(pv);
    }
    else if (pv->pval_type == PVAL_ERROR)
    {
        pval_delete(pv->pval_error);

        free(pv);

    }
    else if (pv->pval_type == PVAL_FUNCTION)
    {
        pv_func* temp_fn = pv->pval_func;
        char* temp_name = pv->pval_func->func_name;

        free(temp_fn);
        free(temp_name);
        free(pv);
    }
    else if (pv->pval_type == PVAL_LIST)
    {
        list_child *head = pv->pval_list->atoms_head;

        while (head != NULL)
        {   
            list_child *temp = head;
            pval_delete(temp->atom);
            head = head->next;

            free(temp);
        }

        free(pv->pval_list);

        free(pv);
    }
};

//grammar rule expression -> atom | list
pval* parse_expression(Node** current) 
{
    if (strcmp((*current)->token, "(") == 0)
    {
        //consume the token -> (
        (*current) = (*current)->next;
        //call parse list?
        return parse_list(current);
    }
    else if (strcmp((*current)->token, ")") == 0)
    {
        char* error_message = "Syntax error, not expecting a )...";
        (*current) = (*current)->next;
        return pval_error(pval_symbol(error_message));
    }
    else
    {
        return parse_atom(current);
    }
};

//grammar rule list -> '(' expression* ')' - * means 0 or more expressions in between ()
pval* parse_list(Node** current)
{
    //create a new empty list to add atoms/other lists to
    pval* new_list = empty_list();

    //(*current) != NULL comes first, because if it's NULL, then the token will also be NULL.
    while (((*current) != NULL) && (strcmp((*current)->token, ")") != 0))
    {
        pval_add(new_list, parse_expression(current));
    }
    
    if (((*current) != NULL) &&  strcmp((*current)->token, ")") == 0)
    {
        (*current) = (*current)->next;
    }

    return new_list;

};

//grammar rule atom -> bool | symbol | number
pval* parse_atom(Node** current) 
{
    //parse booleans #t or #f
    // atom -> bool
    if (strcmp((*current)->token, "#t\0") == 0 || strcmp((*current)->token, "#f\0") == 0) 
    {
        bool temp = false;

        if (strcmp((*current)->token, "#t") == 0) {
            temp = true;
        }
        else if (strcmp((*current)->token, "#f") == 0)
        {
            temp = false;
        }
        pval* b = pval_bool(temp);

        (*current) = (*current)->next;
        return b;
    }
    //parse a number
    // atom -> number
    else if ((((*current)->token[0] == '-' || (*current)->token[0] == '+') && ((*current)->token[1] >= 48 && (*current)->token[1] <= 57)) || ((*current)->token[0] >= 48 && (*current)->token[0] <= 57)) //parse a number
    {
        int64_t temp;

        temp = strtoll((*current)->token, NULL, 10);// sscanf((*current)->token, "%lld", &temp);

        pval* number = pval_number(temp);

        (*current) = (*current)->next;
        return number;
    }
    //parse symbols
    // atom -> symbol
   else 
   {    
        pval* symbol = pval_symbol(((*current)->token));

        (*current) = (*current)->next;
        return symbol;
   }
};