#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <ctype.h>

#include "../include/pvals.h"
#include "../include/env.h"
// #include "../include/eval.h"

pval* pval_copy(pval* pv)
{
    if (pv->pval_type == PVAL_BOOL)
    {
        return pval_bool(pv->pval_bool);
    }
    else if(pv->pval_type == PVAL_NUMBER)
    {
        return pval_number(pv->pval_number);
    }
    else if (pv->pval_type == PVAL_SYMBOL)
    {
        return pval_symbol(pv->pval_symbol);
    }
    else if (pv->pval_type == PVAL_ERROR)
    {
        return pval_error(pval_copy(pv->pval_error));
    }
    else if (pv->pval_type == PVAL_STRING)
    {
        return pval_string(pv->pval_string->str, pv->pval_string->str_len);
    }
    else if (pv->pval_type == PVAL_FUNCTION)
    {
        pv->reference_count++;
        return pv;
    }
    else if(pv->pval_type == PVAL_CLOSURE)
    {
        pv->reference_count++;
        return pv;
    }
    else if (pv->pval_type == PVAL_CELL)
    {
        pv->reference_count++;
        return pv;
    }
    else if(pv->pval_type == PVAL_LIST)
    {
        pval* new_list = empty_list();

        list_child *temp = pv->pval_list->atoms_head;
        while (temp != NULL)
        {
            pval_add(new_list, pval_copy(temp->atom));
            temp = temp->next;
        }

        return new_list;
    }
    else
    {   
        return pval_error(pval_symbol("uncopyable pval"));
    }
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
            printf("#t");
        }
        else
        {
            printf("#f");
        }
    }
    else if (pv->pval_type == PVAL_NUMBER)
    {
        printf("%lld", (long long)pv->pval_number);
    }
    else if (pv->pval_type == PVAL_SYMBOL)
    {
        printf("%s", pv->pval_symbol);
    }
    else if (pv->pval_type == PVAL_STRING)
    {   
        printf("%c", '\"');
        for (int j=0; j<pv->pval_string->str_len; j++)
        {
            if(pv->pval_string->str[j] == '\n')
            {
                printf("%c%c", '\\', 'n');
            }
            else if(pv->pval_string->str[j] == '\\')
            {
                printf("%c%c", '\\', '\\');
            }
            else if(pv->pval_string->str[j] == '\0')
            {
                printf("%c%c%c%c", '\\', 'x', '0', '0');
            }
            else if(pv->pval_string->str[j] == '\"')
            {
                printf("%c%c", '\\', '\"');
            }
            else if ((pv->pval_string->str[j] >= 32) && (pv->pval_string->str[j] <= 126))
            {
                printf("%c", pv->pval_string->str[j]);
            }
            else
            {
                printf("%c%c%02x", '\\', 'x', (unsigned char)pv->pval_string->str[j]);
            }
        }
        printf("%c", '\"');
    }
    else if (pv->pval_type == PVAL_ERROR)
    {
        printf("$error{");
        pval_print(pv->pval_error);
        printf("}");
    }
    else if (pv->pval_type == PVAL_FUNCTION)
    {
        printf("$builtin{%s}", pv->pval_func->func_name);
    }
    else if (pv->pval_type == PVAL_LIST)
    {
        // case for printing out quote to ' if quote is a symbol
        if (pv->pval_list->atoms_head != NULL && pv->pval_list->atoms_head->atom->pval_type == PVAL_SYMBOL)
        {
            if (((pv->pval_list->atoms_head->next != NULL)) && (strcmp(pv->pval_list->atoms_head->atom->pval_symbol, "quote") == 0))
            {
                printf("'");
                pval_print(pv->pval_list->atoms_head->next->atom);
                return;
            }
        }
        printf("(");
        list_child *temp = pv->pval_list->atoms_head;
        int i = 0;
        while (temp != NULL) 
        {
            if (i != 0) { printf(" "); } 
            pval_print(temp->atom);
            i++;
            temp = temp->next;
        }
        printf(")");
    }
    else if (pv->pval_type == PVAL_CLOSURE)
    {
        if (pv->pval_closure->is_macro == true)
        {
            printf("$macro{");
        }
        else {
            printf("$lambda{");
        }
        pval_print(pv->pval_closure->arglist);
        printf(" ");
        pval_print(pv->pval_closure->body);
        printf("}");
        if (pv->pval_closure->fn_name != NULL)
        {
            printf("@");
            pval_print(pv->pval_closure->fn_name);
        }
    }
    else if (pv->pval_type == PVAL_CELL)
    {
        printf("$cell{");
        pval_print(pv->pval_cell);
        printf("}@%p", (void *)pv);
    }
};



void pval_delete(pval* pv)
{
    if (pv == NULL)
    {
        return;
    }
    //decrement the reference count, if == 0, then delete the pval, else return.
    pv->reference_count--;
    if (pv->reference_count > 0)
    {
        return;
    }
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
    else if (pv->pval_type == PVAL_CELL)
    {
        pval_delete(pv->pval_cell);

        free(pv);
    }
    else if (pv->pval_type == PVAL_FUNCTION)
    {
        pv_func* temp_fn = pv->pval_func;
        char* temp_name = pv->pval_func->func_name;

        free(temp_name);
        free(temp_fn);
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
    else if (pv->pval_type == PVAL_STRING)
    {
        pv_str *temp_pv_str = pv->pval_string;
        char *temp_str = temp_pv_str->str;
        free(temp_str);
        free(temp_pv_str);

        free(pv);
    }
    else if (pv->pval_type == PVAL_CLOSURE)
    {
        pval* temp_fn = pv->pval_closure->fn_name;
        pval* temp_args = pv->pval_closure->arglist;
        pval* temp_body = pv->pval_closure->body;

        environment *temp_env = pv->pval_closure->env;

        closure *temp_closure = pv->pval_closure;

        pval_delete(temp_fn);
        pval_delete(temp_args);
        pval_delete(temp_body);

        delete_env_chain(temp_env);

        free(temp_closure);
        free(pv);
    }
};