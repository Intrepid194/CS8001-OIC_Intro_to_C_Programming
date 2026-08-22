#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <ctype.h>

#include "../include/pvals.h"
#include "../include/env.h"

pval* pval_closure(bool is_macro, pval* fn_name, pval* arglist, pval* body, environment* env)
{
    pval* pv_closure = (pval *)malloc(sizeof(pval));
    pv_closure->pval_type = PVAL_CLOSURE;
    pv_closure->reference_count = 1;

    //allocate memory for a new pval closure
    closure* pv_clos = (closure *)malloc(sizeof(closure));

    //copy fn_name if not NULL, otherwise NULL
    pv_clos->fn_name = NULL;
    if (fn_name != NULL) { pv_clos->fn_name = pval_copy(fn_name); }

    //assign the is_macro flag here
    pv_clos->is_macro = is_macro;

    //assign user defined function arguments to the closure,
    //need to pval_copy since the main() frees the evaled_tree which this gets called from
    pv_clos->arglist = pval_copy(arglist);

    //assign user defined function body to the closure,
    //need to pval_copy since the main() frees the evaled_tree which this gets called from    
    pv_clos->body = pval_copy(body);

    //do not copy environment here since I will have to deep copy the environment in other places besides just here
    //so do copying outside, then pass frozen copy into the closure
    pv_clos->env = env;

    pv_closure->pval_closure = pv_clos;

    return pv_closure;
}

pval* pval_func(pval_function pval_fn, char* func_name)
{
    pv_func *fn_struct = (pv_func *)malloc(sizeof(pv_func));

    fn_struct->func_name = strdup(func_name);
    fn_struct->pval_fn_def = pval_fn;

    pval* fn = (pval *)malloc(sizeof(pval));

    fn->reference_count = 1;
    fn->pval_type = PVAL_FUNCTION;
    fn->pval_func = fn_struct;
   
    return fn;
}

pval* pval_cell(pval* x)
{
    pval* pv_cell = (pval *)malloc(sizeof(pval));

    pv_cell->reference_count = 1;
    pv_cell->pval_type = PVAL_CELL;
    pv_cell->pval_cell = x;

    return pv_cell;
}

pval* pval_number(int64_t n) 
{
    pval *pval_num = (pval *)malloc(sizeof(pval));
    
    pval_num->reference_count = 1;
    pval_num->pval_type = PVAL_NUMBER;
    pval_num->pval_number = n;

    return pval_num;
};

pval* pval_bool(bool b) 
{
    pval* pval_bl = (pval *)malloc(sizeof(pval));

    pval_bl->reference_count = 1;
    pval_bl->pval_type = PVAL_BOOL;
    pval_bl->pval_bool = b;
    
    return pval_bl;
};

pval* pval_symbol(char *symbol)
{
    pval* pval_sym = (pval *)malloc(sizeof(pval));

    pval_sym->reference_count = 1;
    pval_sym->pval_type = PVAL_SYMBOL;


    //duplicate the symbol tokens in the tokenized list so the token list can be freed after it is parsed.
    //need to free this symbol when defining pval_delete().
    pval_sym->pval_symbol = strdup(symbol);

    return pval_sym;
};

pval* pval_string(char *str, int str_len)
{
    pval* pval_str = (pval *)malloc(sizeof(pval));

    pval_str->reference_count = 1;
    pval_str->pval_type = PVAL_STRING;

    pv_str* pv_string = (pv_str *)malloc(sizeof(pv_str));
    pv_string->str_len = str_len;
    pv_string->str = (char *)malloc((str_len+1)*sizeof(char));
    memcpy((pv_string->str), str, str_len);
    pv_string->str[str_len] = '\0';

    pval_str->pval_string = pv_string;

    return pval_str;
}


pval* pval_error(pval* x)
{
    pval *error = (pval *)malloc(sizeof(pval));

    error->reference_count = 1;
    error->pval_type = PVAL_ERROR;
    error->pval_error = x;

    return error;
};

//makes empty list to add pvals to later by calling pval_add().
pval* empty_list()
{
    pval *em_list = (pval *)malloc(sizeof(pval));

    em_list->reference_count = 1;
    em_list->pval_type = PVAL_LIST;

    list *new_list = (list *)malloc(sizeof(list));
    new_list->list_count = 0;
    new_list->atoms_head = NULL;

    em_list->pval_list = new_list;

    return em_list;
};