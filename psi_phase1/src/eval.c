#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include <errno.h>

#include "../include/eval.h"
#include "../include/globals.h"


bool check_protected_symbols(char* name)
{
    const char *protected_syms[] = {"+", "-", "*", "/", "=", ">", "<", ">=", "<=", "!=", "quit", "head", "tail", "cons", "not", "type", "ord", "chr", "input" ,"output", "cell", "!", ":=", "str", "print", "error", "eval", "read", "load", "get-file", "put-file"};
    int length = sizeof(protected_syms) / sizeof(protected_syms[0]);

    for (int i = 0; i < length; i++)
    {
        if (strcmp(name, protected_syms[i]) == 0)
        {
            return true;
        }
    }
    return false;
}

pval* protected_symbol_error(pval* value)
{
    pval* err_list = empty_list();
    pval_add(err_list, pval_symbol("protected-symbol"));
    pval_add(err_list, pval_copy(value));
    return pval_error(err_list);
};

pval* arity_error(char* func, char* cmp_symbol, int64_t exp_args, int64_t actual_args)
{
    pval* err_list = empty_list();
    pval_add(err_list, pval_symbol("arity-error"));
    pval_add(err_list, pval_symbol(func));
    
    pval* cmp_list = empty_list();
    pval_add(cmp_list, pval_symbol(cmp_symbol));
    pval_add(cmp_list, pval_number(exp_args));
    pval_add(err_list, cmp_list);
    pval_add(err_list, pval_number(actual_args));
    return pval_error(err_list);  
}

pval* arity_error_btwn_2_args(char* func, char* cmp_symbol, int64_t actual_args)
{
    pval* err_list = empty_list();

    pval_add(err_list, pval_symbol("arity-error"));
    pval_add(err_list, pval_symbol(func));

    pval* cmp_list = empty_list();

    pval_add(cmp_list, pval_symbol(cmp_symbol));
    pval_add(cmp_list, pval_number(2));
    pval_add(cmp_list, pval_number(3));
    pval_add(err_list, cmp_list);

    pval_add(err_list, pval_number(actual_args));
    
    return pval_error(err_list); 
}

pval* type_error(char* func, char* exp_type, int64_t position, pval* value)
{
    pval* err_list = empty_list();

    pval_add(err_list, pval_symbol("type-error"));
    pval_add(err_list, pval_symbol(func));
    pval_add(err_list, pval_number(position));
    pval_add(err_list, pval_symbol(exp_type));

    pval_add(err_list, pval_copy(value));

    return pval_error(err_list);
};

pval* value_error(char* func, pval* value)
{
    pval* err_list = empty_list();

    pval_add(err_list, pval_symbol("value-error"));
    pval_add(err_list, pval_symbol(func));
    pval_add(err_list, pval_copy(value));

    return pval_error(err_list);
};

pval* file_error(char *err_type, pval* filename)
{
    pval* err_list = empty_list();
    pval_add(err_list, pval_symbol(err_type));
    pval_add(err_list, pval_string(strdup(filename->pval_string->str), filename->pval_string->str_len));

    char *file_err = strerror(errno);

    pval_add(err_list, pval_string(file_err, strlen(file_err)));

    return pval_error(err_list);  
}

pval* arglist_error(char* func, char *err_type, int64_t position, pval* value)
{
    pval* err_list = empty_list();

    pval_add(err_list, pval_symbol("arglist-error"));
    pval_add(err_list, pval_symbol(func));
    pval_add(err_list, pval_symbol(err_type));

    if (strcmp("bad-&-position", err_type) == 0)
    {
        pval_add(err_list, pval_number(position));
        return pval_error(err_list);
    }

    pval_add(err_list, pval_copy(value));

    return pval_error(err_list);
}

pval* new_symbol(pval* list)
{
    if (list->pval_list->list_count != 0)
    {
        return arity_error("new-symbol", "=", 0, list->pval_list->list_count);
    }

    new_sym_inc++;
    int length = snprintf(NULL, 0, "%" PRId64, new_sym_inc);
    char * temp_symbol = (char *) malloc((length+3)*sizeof(char));

    snprintf(temp_symbol, length+3, "g$%" PRId64, new_sym_inc);

    pval* pv_sym = pval_symbol(temp_symbol);
    free(temp_symbol);
    return pv_sym;
}

pval* add(pval* list)
{
    list_child* temp = list->pval_list->atoms_head;

    int64_t sum = 0;

    int64_t i = 1;
    while(temp != NULL)
    {
        if (temp->atom->pval_type == PVAL_NUMBER)
        {
            sum = sum + temp->atom->pval_number;
        }
        else
        {
            return type_error("+", "number", i, temp->atom);
        }
        i++;
        temp = temp->next;
    }

    return pval_number(sum);
}

pval* subtract(pval* list)
{
    list_child* temp = list->pval_list->atoms_head;

    //checks for arity 0
    if (list->pval_list->list_count < 1)
    {   
        return arity_error("-", ">=", 1, list->pval_list->list_count);
    }

    int64_t difference = 0;
    int64_t count = 0;
    
    int64_t i = 1;
    while (temp != NULL)
    {   
        if (temp->atom->pval_type == PVAL_NUMBER)
        {
            if (count == 0)
            {
                difference = temp->atom->pval_number;
            } 
            else
            {
                difference =  difference - temp->atom->pval_number;
            }
        }
        else
        {
            return type_error("-", "number", i, temp->atom);
        }
        i++;
        count++;
        temp = temp->next;        
    }

    //check for arity 1
    //if count is 1, then while loop only ran 1 time.
    if (count == 1)
    {
        difference = 0 - difference;
    }

    return pval_number(difference);
};

pval* multiply(pval* list)
{
    list_child* temp = list->pval_list->atoms_head;

    int64_t product = 1;

    int64_t i = 1;
    while (temp != NULL)
    {   
        if (temp->atom->pval_type == PVAL_NUMBER)
        {
            product = product * temp->atom->pval_number;
        }
        else
        {
            return type_error("*", "number", i, temp->atom);
        }
        i++;
        temp = temp->next;        
    }

    return pval_number(product);
};

pval* divide(pval* list)
{
    list_child* temp = list->pval_list->atoms_head;

    int64_t quotient = 0;
    int64_t count = 0;

    int64_t i = 1;

    while (temp != NULL)
    {
        if (temp->atom->pval_type == PVAL_NUMBER)
        {           
            if (count == 0)
            {
                quotient = temp->atom->pval_number;
            }
            else 
            {
                if (temp->atom->pval_number == 0)
                {
                    return pval_error(pval_symbol("division-by-zero"));
                }
                
                else
                {
                    if ((quotient == INT64_MIN) && (temp->atom->pval_number == -1))
                    {
                        return pval_error(pval_symbol("division-INT64_MIN-by-neg-1"));                    
                    }
                    if((quotient ^ temp->atom->pval_number) < 0 && (quotient % temp->atom->pval_number != 0)) {
                        quotient = quotient / temp->atom->pval_number;
                        quotient--;
                    }
                    else
                    {   
                        quotient = quotient / temp->atom->pval_number;
                        
                    }
                }       
            }
        }
        else 
        {
            return type_error("/", "number", i, temp->atom);
        }
        i++;
        count++;
        temp = temp->next;
        
    }
    if (count < 2)
    {
        return arity_error("/", ">=", 2, list->pval_list->list_count);
    }

    return pval_number(quotient);
};

pval* equals(pval* list)
{
    list_child* temp = list->pval_list->atoms_head;

    bool b = true;
    if (temp != NULL)
    {
        if (temp->atom->pval_type == PVAL_NUMBER)
        {
            int64_t reference = temp->atom->pval_number;
            temp = temp->next;

            while (temp != NULL)
            {
                if (temp->atom->pval_type == PVAL_NUMBER)
                {   
                    if (reference != temp->atom->pval_number) 
                    { 
                        b = false;
                        return pval_bool(b); 
                    }
                }
                else
                { 
                    b = false;
                    return pval_bool(b); 
                }
                temp = temp->next;
            }
        }
        else if (temp->atom->pval_type == PVAL_BOOL)
        {
            bool reference = temp->atom->pval_bool;
            temp = temp->next;
            
            while (temp != NULL)
            {
                if (temp->atom->pval_type == PVAL_BOOL)
                {
                    if (reference != temp->atom->pval_bool)
                    {
                        b = false;
                        return pval_bool(b);
                    }
                }
                else
                {   
                    b = false;
                    return pval_bool(b);
                }
                temp = temp->next;
            }
        }
        else if (temp->atom->pval_type == PVAL_SYMBOL)
        {
            char* reference = temp->atom->pval_symbol;
            temp = temp->next;

            while (temp != NULL)
            {
                if (temp->atom->pval_type == PVAL_SYMBOL)
                {   
                    if (strcmp(reference, temp->atom->pval_symbol) != 0)
                    {
                        b = false;
                        return pval_bool(b);
                    }
                }
                else
                {   
                    b = false;
                    return pval_bool(b);
                }
                temp = temp->next;
            }
            
        }
        else if (temp->atom->pval_type == PVAL_STRING)
        {
            pv_str *reference = temp->atom->pval_string;
            temp = temp->next;

            while (temp != NULL)
            {
                if (temp->atom->pval_type == PVAL_STRING)
                {
                    if (temp->atom->pval_string->str_len != reference->str_len)
                    {
                        b = false;
                        return pval_bool(b);
                    }
                    if (memcmp(temp->atom->pval_string->str, reference->str, reference->str_len) != 0)
                    {
                        b = false;
                        return pval_bool(b);
                    }
                }
                else
                {   
                    b = false;
                    return pval_bool(b);
                }
                temp = temp->next;
            }
        }
        else if(temp->atom->pval_type == PVAL_LIST)
        {
            struct list *reference = temp->atom->pval_list;
            temp = temp->next;

            while (temp != NULL)
            {
                if (temp->atom->pval_type == PVAL_LIST)
                {
                    //checks if list lengths are the same
                    if (temp->atom->pval_list->list_count != reference->list_count)
                    {
                        b = false;
                        return pval_bool(b);
                    }
                    list_child* cmp_list = temp->atom->pval_list->atoms_head;
                    list_child* ref_list = reference->atoms_head;

                    while (cmp_list != NULL)
                    {
                        pval* temp_list = empty_list();
                        pval_add(temp_list, pval_copy(ref_list->atom));
                        pval_add(temp_list, pval_copy(cmp_list->atom));

                        
                        pval* bl = equals(temp_list);
                        if (bl->pval_bool == false)
                        {
                            pval_delete(temp_list);
                            return bl;
                        }
                        pval_delete(bl);
                        pval_delete(temp_list);
                        cmp_list = cmp_list->next;
                        ref_list = ref_list->next;
                    }
                }
                else
                {
                    b = false;
                    return pval_bool(b);
                }
                temp = temp->next;
            }
        }
        else if (temp->atom->pval_type == PVAL_FUNCTION)
        {
            pval* reference = temp->atom; 
            temp = temp->next;

            while (temp != NULL)
            {
                if (temp->atom->pval_type == PVAL_FUNCTION)
                {
                    if (temp->atom != reference)
                    {
                        b = false;
                        return pval_bool(b);
                    }                   
                }
                else
                {
                    b = false;
                    return pval_bool(b);
                }
                temp = temp->next;
            }
        }
        else if (temp->atom->pval_type == PVAL_CLOSURE)
        {
            pval* reference = temp->atom;
            temp = temp->next;
            while (temp != NULL)
            {
                if (temp->atom->pval_type == PVAL_CLOSURE)
                {   
                    
                    if (temp->atom != reference)
                    {
                        b = false;
                        return pval_bool(b);
                    }                   
                }
                else
                {
                    b = false;
                    return pval_bool(b);
                }
                temp = temp->next;
            }            
        }
        else if (temp->atom->pval_type == PVAL_CELL)
        {
            pval* reference = temp->atom;
            temp = temp->next;
            while (temp != NULL)
            {
                if (temp->atom->pval_type == PVAL_CELL)
                {   
                    
                    if (temp->atom != reference)
                    {
                        b = false;
                        return pval_bool(b);
                    }                   
                }
                else
                {
                    b = false;
                    return pval_bool(b);
                }
                temp = temp->next;
            }            
        }
        else
        {
            b = false;
            return pval_bool(b);
        }
        
    }
    return pval_bool(b);
};

pval* gt(pval* list)
{
    list_child *temp = list->pval_list->atoms_head;

    bool b = true;

    int64_t i = 1;
    if (temp != NULL)
    {
        if (temp->atom->pval_type == PVAL_NUMBER)
        {
            int64_t reference = temp->atom->pval_number;
            temp = temp->next;

            while (temp != NULL)
            {
                if (temp->atom->pval_type == PVAL_NUMBER)
                {
                    if (reference <= temp->atom->pval_number)
                    {   
                        b = false;
                        return pval_bool(b);
                    }
                    else
                    {
                        reference = temp->atom->pval_number;
                    }
                }
                else
                {
                    return type_error(">", "number", i, temp->atom);
                }
                i++;
                temp = temp->next;
            }
        }
        else{
            return type_error(">", "number", i, temp->atom);
        }
    }
    return pval_bool(b);
};

pval* lt(pval* list)
{
    list_child *temp = list->pval_list->atoms_head;

    bool b = true;
    int64_t i = 1;

    if (temp != NULL)
    {
        if (temp->atom->pval_type == PVAL_NUMBER)
        {
            int64_t reference = temp->atom->pval_number;
            temp = temp->next;

            while (temp != NULL)
            {
                if (temp->atom->pval_type == PVAL_NUMBER)
                {
                    if (reference >= temp->atom->pval_number)
                    {   
                        b = false;
                        return pval_bool(b);
                    }
                    else
                    {
                        reference = temp->atom->pval_number;
                    }
                }
                else
                {
                    return type_error("<", "number", i, temp->atom);
                }
                i++;
                temp = temp->next;
            }
        }
        else{
            return type_error("<", "number", i, temp->atom);
        }
    }
    return pval_bool(b);
};

pval* gte(pval* list)
{
    list_child *temp = list->pval_list->atoms_head;

    bool b = true;

    int64_t i = 1;
    if (temp != NULL)
    {
        if (temp->atom->pval_type == PVAL_NUMBER)
        {
            int64_t reference = temp->atom->pval_number;
            temp = temp->next;

            while (temp != NULL)
            {
                if (temp->atom->pval_type == PVAL_NUMBER)
                {
                    if (reference < temp->atom->pval_number)
                    {   
                        b = false;
                        return pval_bool(b);
                    }
                    else
                    {
                        reference = temp->atom->pval_number;
                    }
                }
                else
                {
                    return type_error(">=", "number", i, temp->atom);
                }
                i++;
                temp = temp->next;
            }
        }
        else{
            return type_error(">=", "number", i, temp->atom);
        }
    }
    return pval_bool(b);
};

pval* lte(pval* list)
{
    list_child *temp = list->pval_list->atoms_head;

    bool b = true;

    int64_t i = 1;
    if (temp != NULL)
    {
        if (temp->atom->pval_type == PVAL_NUMBER)
        {
            int64_t reference = temp->atom->pval_number;
            temp = temp->next;

            while (temp != NULL)
            {
                if (temp->atom->pval_type == PVAL_NUMBER)
                {
                    if (reference > temp->atom->pval_number)
                    {   
                        b = false;
                        return pval_bool(b);
                    }
                    else
                    {
                        reference = temp->atom->pval_number;
                    }
                }
                else
                {
                    return type_error("<=", "number", i, temp->atom);
                }
                temp = temp->next;
            }
        }
        else{
            return type_error("<=", "number", i, temp->atom);
        }
    }
    return pval_bool(b);
};

pval* not_eq(pval* list)
{   
    if (list->pval_list->atoms_head != NULL)
    {
        pval* b = equals(list);
        if (b->pval_bool == false) 
        {
            pval_delete(b);
            return pval_bool(true);
        }
        pval_delete(b);
        return pval_bool(false);
    }
    return pval_bool(false);
};

pval* modulo(pval* list)
{
    if (list->pval_list->list_count != 2)
    {
        return arity_error("%", "=", 2, list->pval_list->list_count);  
    }

    if (list->pval_list->atoms_head->atom->pval_type != PVAL_NUMBER)
    {
        return type_error("%", "number", 1, list->pval_list->atoms_head->next->atom); 
    }

    if (list->pval_list->atoms_head->next->atom->pval_type != PVAL_NUMBER)
    {
        return type_error("%", "number", 2, list->pval_list->atoms_head->next->atom);
    }

    if (list->pval_list->atoms_head->next->atom->pval_number == 0)
    {
        return value_error("%", list->pval_list->atoms_head->next->atom);
    }

    if ((list->pval_list->atoms_head->atom->pval_number == INT64_MIN) && (list->pval_list->atoms_head->next->atom->pval_number == -1))
    {
        return value_error("%", list->pval_list->atoms_head->next->atom);   
    }

    pval* quotient = divide(list);
    int64_t remainder = list->pval_list->atoms_head->atom->pval_number-quotient->pval_number*list->pval_list->atoms_head->next->atom->pval_number;

    pval_delete(quotient);
    return pval_number(remainder);
}

pval* head(pval* list)
{
    if (list->pval_list->list_count != 1)
    {
        return arity_error("head", "=", 1, list->pval_list->list_count);
    }
    if (list->pval_list->atoms_head->atom->pval_type == PVAL_LIST && list->pval_list->atoms_head->atom->pval_list->list_count == 0)
    {
        return value_error("head", list->pval_list->atoms_head->atom);
    };
    if (list->pval_list->atoms_head->atom->pval_type != PVAL_LIST) {
        return type_error("head", "list", 1, list->pval_list->atoms_head->atom);
    }
    return pval_copy(list->pval_list->atoms_head->atom->pval_list->atoms_head->atom);
};

pval* tail(pval* list)
{
    if (list->pval_list->list_count != 1)
    {
        return arity_error("tail", "=", 1, list->pval_list->list_count);
    }
    if (list->pval_list->atoms_head->atom->pval_type == PVAL_LIST && list->pval_list->atoms_head->atom->pval_list->list_count == 0)
    {
        return value_error("tail", list->pval_list->atoms_head->atom);
    };
    if (list->pval_list->atoms_head->atom->pval_type != PVAL_LIST) {
        
        return type_error("tail", "list", 1, list->pval_list->atoms_head->atom); 
    }

    pval* new_list = empty_list();
    list_child* temp = list->pval_list->atoms_head->atom->pval_list->atoms_head->next;
    
    while (temp != NULL)
    {
        pval_add(new_list, pval_copy(temp->atom));
        temp = temp->next;
    }

    return new_list;
};

pval* cons(pval* list)
{
    if (list->pval_list->list_count != 2)
    {
        return arity_error("cons", "=", 2, list->pval_list->list_count);
    }

    if (list->pval_list->atoms_head->next->atom->pval_type != PVAL_LIST)
    {
        return type_error("cons", "list", 2, list->pval_list->atoms_head->next->atom);
    }

    pval* new_list = empty_list();
    pval_add(new_list, pval_copy(list->pval_list->atoms_head->atom));

    list_child* temp = list->pval_list->atoms_head->next->atom->pval_list->atoms_head;

    while (temp != NULL)
    {
        pval_add(new_list, pval_copy(temp->atom));
        temp = temp->next;
    }
    return new_list;
};

pval* not(pval* list)
{
    if (list->pval_list->list_count != 1)
    {
        return arity_error("not", "=", 1, list->pval_list->list_count);
    }

    bool b = false;
    if (list->pval_list->atoms_head->atom->pval_type == PVAL_BOOL)
    {
        if (list->pval_list->atoms_head->atom->pval_bool == false)
        {
            b = true;
            return pval_bool(b);
        }
    }
    return pval_bool(b);
};

pval* type(pval* list)
{
    if (list->pval_list->list_count != 1)
    {
        return arity_error("type", "=", 1, list->pval_list->list_count);  
    }

    if (list->pval_list->atoms_head->atom->pval_type == PVAL_SYMBOL) 
    {
        return pval_symbol("symbol");
    }
    if (list->pval_list->atoms_head->atom->pval_type == PVAL_STRING) 
    {
        return pval_symbol("string");
    }
    else if (list->pval_list->atoms_head->atom->pval_type == PVAL_BOOL) 
    {
        return pval_symbol("bool");
    }
    else if (list->pval_list->atoms_head->atom->pval_type == PVAL_NUMBER) 
    {
        return pval_symbol("number");
    }
    else if (list->pval_list->atoms_head->atom->pval_type == PVAL_LIST) 
    {
        return pval_symbol("list");
    }
    else if (list->pval_list->atoms_head->atom->pval_type == PVAL_FUNCTION) 
    {
        return pval_symbol("function");
    }
    else if (list->pval_list->atoms_head->atom->pval_type == PVAL_CLOSURE)
    {
        return pval_symbol("function");
    }
        else if (list->pval_list->atoms_head->atom->pval_type == PVAL_CELL)
    {
        return pval_symbol("cell");
    }
    else
    {
        return pval_symbol("undefined");
    }
};
pval* ord(pval* list) 
{
    // check number of arguments being accepted
    if (list->pval_list->list_count != 1)
    {
        return arity_error("ord", "=", 1, list->pval_list->list_count);
    }
    // check argument data types
    if (list->pval_list->atoms_head->atom->pval_type != PVAL_STRING)
    {
        return type_error("ord", "string", 1, list->pval_list->atoms_head->atom);
    }

    pval* new_list = empty_list();

    pv_str *pval_str = list->pval_list->atoms_head->atom->pval_string;

    for (int j=0; j<pval_str->str_len; j++)
    {
        pval_add(new_list, pval_number((int64_t)(unsigned char)pval_str->str[j]));
    }
    return new_list;

};

pval* str_to_chr(pval* list) 
{
    if (list->pval_list->list_count != 1)
    {
        return arity_error("chr", "=", 1, list->pval_list->list_count);
    }
    // check argument data types
    if (list->pval_list->atoms_head->atom->pval_type != PVAL_LIST)
    {
        return type_error("chr", "list", 1, list->pval_list->atoms_head->atom);
    }

    list_child* temp = list->pval_list->atoms_head->atom->pval_list->atoms_head;

    char* temp_str = (char *)malloc((list->pval_list->atoms_head->atom->pval_list->list_count+1)*sizeof(char));

    int i = 0;
    while (temp != NULL)
    {
        if (temp->atom->pval_type != PVAL_NUMBER)
        {
            free(temp_str);
            return type_error("chr", "number", i+1, list->pval_list->atoms_head->atom);
        }
        if (temp->atom->pval_number > 255)
        {
            free(temp_str);
            return value_error("chr", temp->atom);
        }
        if (temp->atom->pval_number < 0)
        {
            free(temp_str);
            return value_error("chr", temp->atom);
        }
        temp_str[i] = (unsigned char)temp->atom->pval_number;
        i++;
        temp = temp->next;
    }

    pval* pv_string = pval_string(temp_str, i);
    free(temp_str);
    return pv_string;
};

pval* input(pval *list) 
{
    if (list->pval_list->list_count != 0)
    {
        return arity_error("input", "=", 0, list->pval_list->list_count);
    }

    char *input = (char *)malloc(4097*sizeof(char));
    
    if (fgets(input, 4097, stdin) == NULL)
    {
        pval* pv_string = pval_string(input, 0);
        free(input);
        return pv_string;
    }
    int input_len = strlen(input);
    
    if (input_len > 0 && input[input_len-1] == '\n') 
    { 
        input[input_len-1] = '\0'; 
        input_len--;
    }

    if (input_len > 0 && input[input_len-1] == '\r') { input[input_len-1] = '\0'; }

    pval* pv_string = pval_string(input, input_len);
    free(input);
    return pv_string;
};

pval* output(pval* list) 
{
    list_child *temp = list->pval_list->atoms_head;

    int64_t i = 1;
    while (temp != NULL)
    {
        if (temp->atom->pval_type != PVAL_STRING)
        {
            return type_error("output", "string", i, list->pval_list->atoms_head->atom);
        }
        i++;
        temp = temp->next;
    }

    temp = list->pval_list->atoms_head;

    while (temp != NULL)
    {
        for (int j = 0; j<temp->atom->pval_string->str_len; j++)
        {
            printf("%c", temp->atom->pval_string->str[j]);
        }
        temp = temp->next;
    }
    return pval_bool(true);
};

pval* create_cell(pval* list) 
{
    if (list->pval_list->list_count != 1)
    {
        return arity_error("cell", "=", 1, list->pval_list->list_count);
    }

    if (list->pval_list->atoms_head->atom->pval_type == PVAL_ERROR)
    {
        return type_error("cell", "non-error", 1, list->pval_list->atoms_head->atom);
    }

    return pval_cell(pval_copy(list->pval_list->atoms_head->atom));
};

pval* deref_cell(pval* list) 
{
    if (list->pval_list->list_count != 1)
    {
        return arity_error("!", "=", 1, list->pval_list->list_count);
    }
    if (list->pval_list->atoms_head->atom->pval_type != PVAL_CELL)
    {
        return type_error("!", "cell", 1, list->pval_list->atoms_head->atom);       
    }

    return pval_copy(list->pval_list->atoms_head->atom->pval_cell);
};

pval* write_cell(pval* list)
{
    if (list->pval_list->list_count != 2)
    {
        return arity_error(":=", "=", 2, list->pval_list->list_count);
    }

    if (list->pval_list->atoms_head->atom->pval_type != PVAL_CELL)
    {
        return type_error(":=", "cell", 1, list->pval_list->atoms_head->atom); 
    }

    pval* temp = list->pval_list->atoms_head->atom->pval_cell;

    list->pval_list->atoms_head->atom->pval_cell = pval_copy(list->pval_list->atoms_head->next->atom);

    pval_delete(temp);
    return pval_copy(list->pval_list->atoms_head->atom->pval_cell);
};

pval* str(pval* list)
{
    if (list->pval_list->list_count != 1)
    {
        return arity_error("str", "=", 1, list->pval_list->list_count);
    }
    pval* pv = list->pval_list->atoms_head->atom;
    if (pv->pval_type == PVAL_BOOL)
    {
        if (pv->pval_bool) return pval_string("#t", 2);
        return pval_string("#f", 2);
    }
    else if (pv->pval_type == PVAL_NUMBER)
    {
        int length = snprintf(NULL, 0, "%" PRId64, pv->pval_number);
        char* temp_str = (char *) malloc((length+1)*sizeof(char));

        snprintf(temp_str, length+1, "%" PRId64, pv->pval_number);

        pval* pv_str = pval_string(temp_str, length);
        free(temp_str);
        return pv_str;
    }
    else if (pv->pval_type == PVAL_SYMBOL)
    {
        int str_len = strlen(pv->pval_symbol);
        return pval_string(pv->pval_symbol, str_len);
    }
    else if (pv->pval_type == PVAL_FUNCTION)
    {
        int len = snprintf(NULL, 0, "$builtin{%s}", pv->pval_func->func_name);
        char *temp_str = (char *)malloc((len+1)*sizeof(char));


        snprintf(temp_str, len+1, "$builtin{%s}", pv->pval_func->func_name);

        pval* pv_str = pval_string(temp_str, len);
        free(temp_str);
        return pv_str;
    }
    else if (pv->pval_type == PVAL_CLOSURE)
    {
        pval* holding_list = empty_list();

        int len = 2;
        if (pv->pval_closure->is_macro == true)
        {
            pval_add(holding_list, pval_string("$macro{", 7));
            len+=7;
        }
        else {
            pval_add(holding_list, pval_string("$lambda{", 8));
            len+=8;
        }

        pval* temp_list = empty_list();
        pval_add(temp_list, pval_copy(pv->pval_closure->body));

        pval* temp_arglist = empty_list();
        pval_add(temp_arglist, pval_copy(pv->pval_closure->arglist));

        pval* arglist_str = str(temp_arglist);
        pval* body_str = str(temp_list);

        pval_add(holding_list, arglist_str);
        pval_add(holding_list, pval_string(" ", 1));
        pval_add(holding_list, body_str);
        pval_add(holding_list, pval_string("}", 1));

        len+=arglist_str->pval_string->str_len;
        len+=body_str->pval_string->str_len;

        if (pv->pval_closure->fn_name != NULL)
        {
            int sym_len = strlen(pv->pval_closure->fn_name->pval_symbol);
            pval_add(holding_list, pval_string("@", 1));
            pval_add(holding_list, pval_string(pv->pval_closure->fn_name->pval_symbol, sym_len));
            len++; // for @
            len += sym_len;
        }
        char* temp_str = (char *)malloc(len*sizeof(char));
        int idx = 0;
        list_child* temp = holding_list->pval_list->atoms_head;
        while (temp != NULL)
        {   
            memcpy(temp_str+idx, temp->atom->pval_string->str, temp->atom->pval_string->str_len);
            idx += temp->atom->pval_string->str_len;
            temp = temp->next;
        }
        pval* pv_str = pval_string(temp_str, idx);
        pval_delete(holding_list);
        pval_delete(temp_list);
        pval_delete(temp_arglist);

        free(temp_str);
        return pv_str;
    }
    else if (pv->pval_type == PVAL_CELL)
    {

        pval* temp_list = empty_list();
        pval_add(temp_list, pval_copy(pv->pval_cell));

        pval* temp = str(temp_list);
        int temp_len = temp->pval_string->str_len;
        int pt_len = snprintf(NULL, 0, "%p", (void *)temp);
        
        char* temp_str = (char *)malloc((temp_len+pt_len+8+1)*sizeof(char));
        snprintf(temp_str, (temp_len+pt_len+1+8), "$cell{%s}@%p", temp->pval_string->str, (void *)pv);
        
        pval* pv_str = pval_string(temp_str, temp_len+pt_len+8);
        pval_delete(temp_list);
        pval_delete(temp);
        free(temp_str);
        return pv_str;
    }
    else if (pv->pval_type == PVAL_LIST)
    {
        list_child *temp = pv->pval_list->atoms_head;
        
        pval* holding_list = empty_list();

        int len = 3; // for ( ) /0
        while (temp != NULL)
        {
            pval* temp_list = empty_list();
            pval_add(temp_list, pval_copy(temp->atom));
            pval* temp_str = str(temp_list);

            len = len + temp_str->pval_string->str_len + 1;
            pval_add(holding_list, pval_copy(temp_str));
            pval_delete(temp_str);
            pval_delete(temp_list);

            temp = temp->next;
        }
        len--;

        char *temp_str = (char *)malloc(len*sizeof(char));
        int idx = 0;
        temp_str[idx++] = '(';
        temp = holding_list->pval_list->atoms_head;
        while (temp != NULL)
        {   
            if (idx != 1) temp_str[idx++] = ' ';
            memcpy(temp_str+ idx, temp->atom->pval_string->str, temp->atom->pval_string->str_len);
            idx += temp->atom->pval_string->str_len;
            temp = temp->next;
        }
        temp_str[idx++] = ')';
        pval* pv_str = pval_string(temp_str, idx);
        pval_delete(holding_list);
        free(temp_str);
        return pv_str;
    }
    else if (pv->pval_type == PVAL_STRING)
    {
        char *temp_str = (char *)malloc((4*pv->pval_string->str_len+3)*sizeof(char));

        int count = 0;
        temp_str[count++] = '\"';
        for (int j=0; j<pv->pval_string->str_len; j++)
        {
            if(pv->pval_string->str[j] == '\n')
            {
                temp_str[count++] = '\\';
                temp_str[count++] = 'n';
            }
            else if(pv->pval_string->str[j] == '\\')
            {
                temp_str[count++] = '\\';
                temp_str[count++] = '\\';
            }
            else if(pv->pval_string->str[j] == '\0')
            {
                temp_str[count++] = '\\';
                temp_str[count++] = 'x';
                temp_str[count++] = '0';
                temp_str[count++] = '0';
            }
            else if(pv->pval_string->str[j] == '\"')
            {
                temp_str[count++] = '\\';
                temp_str[count++] = '\"';
            }
            else if ((pv->pval_string->str[j] >= 32) && (pv->pval_string->str[j] <= 126))
            {
                temp_str[count++] = pv->pval_string->str[j];
            }
            else
            {
                snprintf(temp_str+count, 5, "\\x%02x", (unsigned char)pv->pval_string->str[j]);
                count+=4;
            }
        }
        temp_str[count++] = '\"';

        pval* pv_str = pval_string(temp_str, count);
        free(temp_str);
        return pv_str;
    }
    else
    {
        pval* temp_list = empty_list();
        pval_add(temp_list, pval_copy(pv->pval_error));

        pval* temp = str(temp_list);
        int temp_len = temp->pval_string->str_len;
       
        char* temp_str = (char *)malloc((temp_len+8+1)*sizeof(char));
        snprintf(temp_str, (temp_len+1+8), "$error{%s}", temp->pval_string->str);
        
        pval* pv_str = pval_string(temp_str, temp_len+8);
        pval_delete(temp_list);
        pval_delete(temp);
        free(temp_str);
        return pv_str;    
    }
};

pval* print(pval* list)
{
    pval* output_list = empty_list();
    list_child *temp = list->pval_list->atoms_head;
    while (temp != NULL)
    {
        pval* tmp_list = empty_list();
        pval_add(tmp_list, pval_copy(temp->atom));
        pval_add(output_list, str(tmp_list));
        if (temp->next != NULL)
        {
            pval_add(output_list, pval_string(" ", 1));
        }
        pval_delete(tmp_list);
        temp = temp->next;
    }
    pval_add(output_list, pval_string("\n", 1));
    pval* otpt = output(output_list);
    pval_delete(output_list);
    return otpt;
};

pval* make_list(pval* list)
{
    return pval_copy(list);
}
pval* error(pval* list)
{
    if (list->pval_list->list_count != 1)
    {
        return arity_error("error", "=", 1, list->pval_list->list_count);
    }

    return pval_error(pval_copy(list->pval_list->atoms_head->atom));
};

Node* parse_help(pval* list)
{
    pval* pv_str = list->pval_list->atoms_head->atom;
    // create NULL terminated copy
    
    char* temp_str = (char *)malloc((pv_str->pval_string->str_len+1)*sizeof(char));

    memcpy(temp_str, pv_str->pval_string->str, pv_str->pval_string->str_len);
    temp_str[pv_str->pval_string->str_len] = '\0';
    Node* token_list = NULL;
    token_list = lex(token_list, temp_str);

    Node* cursor = token_list;
    free(temp_str);
    return cursor;
}

pval* read(pval* list)
{
    if (list->pval_list->list_count != 1)
    {
        return arity_error("read", "=", 1, list->pval_list->list_count);
    }
    if (list->pval_list->atoms_head->atom->pval_type != PVAL_STRING)
    {
        return type_error("read", "string", 1, list->pval_list->atoms_head->atom);
    }

    Node* cursor = parse_help(list);
    
    if (cursor == NULL)
    {
        pval* str = pval_string("", 0);
        pval* err = value_error("read", str);

        pval_delete(str);
        return err;
    }
    Node* head = cursor;

    pval* parse_exp = parse_expression(&cursor);

    if (cursor != NULL)
    {
        char *err_msg = "error: (read) value-error.";
        pval *temp = parse_exp;
        parse_exp = pval_error(pval_symbol(err_msg));
        pval_delete(temp);
    }

    deleteList(head);

    return parse_exp;
};

pval* create_list(pval* list)
{
    return pval_copy(list);
};

pval* get_file(pval* list)
{
    if (list->pval_list->list_count != 1)
    {
        return arity_error("get-file", "=", 1, list->pval_list->list_count);
    }
    if (list->pval_list->atoms_head->atom->pval_type != PVAL_STRING)
    {
        return type_error("get-file", "string", 1, list->pval_list->atoms_head->atom);
    }
    pval* filename = list->pval_list->atoms_head->atom;
    FILE *fptr;
    fptr = fopen(filename->pval_string->str, "rb");

    if (fptr == NULL)
    {
        pval* err = file_error("bad-filename", filename);
        fclose(fptr);
        pval_delete(filename);
        return err;
    }

    fseek(fptr, 0L, SEEK_END);
    int64_t fsize = ftell(fptr);

    if (fsize < 0)
    {
        pval* err = file_error("file-error", filename);
        fclose(fptr);
        pval_delete(filename);
        return err;                         
    }

    rewind(fptr);

    char *buffer = (char *)malloc((fsize)*sizeof(char));
    
    int64_t n = fread(buffer, 1, fsize, fptr);
    fclose(fptr);

    pval* buffer_str = pval_string(buffer, n);
    free(buffer);
    return buffer_str;
};

pval* put_file(pval* list)
{
    if (list->pval_list->list_count != 2)
    {
        return arity_error("put-file", "=", 2, list->pval_list->list_count);
    }
    list_child *temp = list->pval_list->atoms_head;

    int i = 0;
    while (temp != NULL)
    {
        if (temp->atom->pval_type != PVAL_STRING)
        {
            return type_error("put-file", "string", i, temp->atom);
        }
        i++;
        temp = temp->next;
    }

    pval* filename = list->pval_list->atoms_head->atom;
    FILE *fptr;
    fptr = fopen(filename->pval_string->str, "wb");

    if (fptr == NULL)
    {
        pval* err = file_error("bad-filename", filename);
        fclose(fptr);
        pval_delete(filename);
        return err;  
    }

    pval* str_to_write = list->pval_list->atoms_head->next->atom;

    fwrite(str_to_write->pval_string->str, 1, str_to_write->pval_string->str_len, fptr);
    fclose(fptr);
    return pval_bool(true);
};

pval* exit_prg(pval* list)
{
    list_child* temp = list->pval_list->atoms_head;

    if (temp != NULL)
    {
        return arity_error("quit", "=", 0, list->pval_list->list_count);
    }
    terminate = true;
    return pval_number(0);
};

pval* eval_tree(environment* env, pval* x)
{
    if (x->pval_type == PVAL_BOOL || x->pval_type == PVAL_NUMBER || x->pval_type == PVAL_ERROR || x->pval_type == PVAL_FUNCTION || x->pval_type == PVAL_STRING || x->pval_type == PVAL_CLOSURE || x->pval_type == PVAL_CELL)
    {
        return pval_copy(x);
    }
    else if (x->pval_type == PVAL_SYMBOL)
    {   
        return lookup_env_binding(env, x->pval_symbol);
    }
    else if (x->pval_type == PVAL_LIST)
    {
        if (x->pval_list->atoms_head == NULL)
        {
            return pval_copy(x);
        }
        else
        {
            //add def (special form) here 
            //checks if the first element in the list is a symbol
            if (x->pval_list->atoms_head->atom->pval_type == PVAL_SYMBOL) {
                //checks if the symbol is "def"
                if (strcmp(x->pval_list->atoms_head->atom->pval_symbol, "def") == 0)
                {
                    if (x->pval_list->list_count == 3)
                    {
                        if (x->pval_list->atoms_head->next->atom->pval_type != PVAL_SYMBOL)
                        {
                            return type_error("def", "symbol", 2, x->pval_list->atoms_head->next->atom);
                        }
                        if (check_protected_symbols(x->pval_list->atoms_head->next->atom->pval_symbol))
                        {
                            return protected_symbol_error(x->pval_list->atoms_head->next->atom);
                        }

                        pval *eval_def = eval_tree(env, x->pval_list->atoms_head->next->next->atom);
                        if (eval_def->pval_type == PVAL_ERROR)
                        {
                           return eval_def;
                        }
                        env_add_binding(env, create_binding(x->pval_list->atoms_head->next->atom->pval_symbol, eval_def));
                        return pval_copy(eval_def);
                    }
                    else if (x->pval_list->list_count == 4)
                    {
                        if (x->pval_list->atoms_head->next->atom->pval_type != PVAL_SYMBOL)
                        {
                            return type_error("def", "symbol", 2, x->pval_list->atoms_head->next->atom);
                        }
                        if (check_protected_symbols(x->pval_list->atoms_head->next->atom->pval_symbol))
                        {
                            return protected_symbol_error(x->pval_list->atoms_head->next->atom);
                        }

                        pval* def_list = empty_list();
                        char *fn = "fn";
                        pval_add(def_list, pval_symbol(fn));

                        list_child *temp = x->pval_list->atoms_head->next;
                        while (temp != NULL)
                        {
                            pval_add(def_list, pval_copy(temp->atom));
                            temp = temp->next;
                        }
                        pval *eval_def_lst = eval_tree(env, def_list);
                        pval_delete(def_list);

                        if (eval_def_lst->pval_type != PVAL_ERROR)
                        {
                            env_add_binding(env, create_binding(x->pval_list->atoms_head->next->atom->pval_symbol, eval_def_lst));
                            return pval_copy(eval_def_lst);
                        }
                        return eval_def_lst;
                    }
                    else
                    {
                        return arity_error_btwn_2_args("def", "=", x->pval_list->list_count);
                    }
                }
                else if (strcmp(x->pval_list->atoms_head->atom->pval_symbol, "if") == 0)
                {
                    if (x->pval_list->list_count == 3 || x->pval_list->list_count == 4)
                    {
                        //evaluate 1st subform of if statement
                        pval* eval_first = eval_tree(env, x->pval_list->atoms_head->next->atom);
                        if (eval_first->pval_type == PVAL_ERROR) { return eval_first; };

                        if (eval_first->pval_type == PVAL_BOOL)
                        {
                            if (eval_first->pval_bool == false)
                            {
                                if (x->pval_list->list_count == 3) 
                                { 
                                    pval_delete(eval_first);
                                    return pval_bool(false);
                                }
                                //eval 3rd subform if there is a 3rd subform and 1st subform is false
                                pval* eval_third = eval_tree(env, x->pval_list->atoms_head->next->next->next->atom);
                                pval_delete(eval_first);
                                return eval_third;
                            }
                        }
                        //eval 2nd subform if 1st subform is true
                        pval *eval_second = eval_tree(env, x->pval_list->atoms_head->next->next->atom);
                        pval_delete(eval_first);
                        return eval_second;
                    }
                    else
                    {
                        return arity_error_btwn_2_args("if", "=", x->pval_list->list_count);
                    }
                }
                else if (strcmp(x->pval_list->atoms_head->atom->pval_symbol, "quote") == 0)
                {
                    if (x->pval_list->list_count == 2) {
                        return pval_copy(x->pval_list->atoms_head->next->atom);
                    }
                    else
                    {
                        return arity_error("quote", "=", 1, x->pval_list->list_count);
                    }
                }
                else if(strcmp(x->pval_list->atoms_head->atom->pval_symbol, "fn") == 0)
                {
                    if ((x->pval_list->list_count != 3) && (x->pval_list->list_count != 4))
                    {
                        return arity_error_btwn_2_args("fn", "=", x->pval_list->list_count);                 
                    }

                    //anonymous user defined functions
                    //fn is first element
                    if (x->pval_list->list_count == 3)
                    {
                        // check that arglist is a list
                        pval* arglist = x->pval_list->atoms_head->next->atom;
                        if (arglist->pval_type != PVAL_LIST)
                        {
                            return arglist_error("fn", "not-list", -1, arglist);
                        }
                        if (arglist->pval_list->atoms_head != NULL)
                        {
                            list_child *temp = arglist->pval_list->atoms_head;

                            //check each element in arg list that it is symbol and that & is second to last
                            int i = 0;
                            while (temp != NULL)
                            {
                                if (temp->atom->pval_type != PVAL_SYMBOL)
                                {
                                    return arglist_error("fn", "not-symbol", -1, temp->atom);
                                }
                                if ((strcmp(temp->atom->pval_symbol, "&") == 0) && (i != arglist->pval_list->list_count-2))
                                {
                                    return arglist_error("fn", "bad-&-position", i, temp->atom);
                                }
                                i++;
                                temp = temp->next;
                            }
                            //check for duplicate symbols in arglist
                            list_child* outer = arglist->pval_list->atoms_head;

                            while (outer->next != NULL)
                            {
                                list_child *inner = outer->next;
                                while (inner != NULL)
                                {
                                    if (strcmp(outer->atom->pval_symbol, inner->atom->pval_symbol) == 0)
                                    {        
                                        return arglist_error("fn", "duplicate-symbol", -1, inner->atom);
                                    }
                                    inner = inner->next;
                                }
                                outer = outer->next;
                            }
                        }
                        
                        return pval_closure(false, NULL, arglist, x->pval_list->atoms_head->next->next->atom, copy_env(env));
                    }
                    //named user defined functions
                    if (x->pval_list->list_count == 4)
                    {
                        if (x->pval_list->atoms_head->next->atom->pval_type != PVAL_SYMBOL)
                        {
                            return type_error("fn", "symbol", 1, x->pval_list->atoms_head->next->atom);
                        }
                        // check that arglist is a list
                        pval* arglist = x->pval_list->atoms_head->next->next->atom;
                        if (arglist->pval_type != PVAL_LIST)
                        {
                            return arglist_error("fn" , "not-list", -1, arglist);
                        }
                        if (arglist->pval_list->atoms_head != NULL)
                        {
                            list_child *temp = arglist->pval_list->atoms_head;

                            //check each element in arg list that it is symbol and that & is second to last
                            int i = 0;
                            while (temp != NULL)
                            {
                                if (temp->atom->pval_type != PVAL_SYMBOL)
                                {
                                    return arglist_error("fn", "not-list", -1, temp->atom);
                                }
                                if ((strcmp(temp->atom->pval_symbol, "&") == 0) && (i != arglist->pval_list->list_count-2))
                                {
                                    return arglist_error("fn", "bad-&-position", i, temp->atom);
                                }
                                i++;
                                temp = temp->next;
                            }
                            //check for duplicate symbols in arglist
                            list_child* outer = arglist->pval_list->atoms_head;

                            while (outer->next != NULL)
                            {
                                list_child *inner = outer->next;
                                while (inner != NULL)
                                {
                                    if (strcmp(outer->atom->pval_symbol, inner->atom->pval_symbol) == 0)
                                    {
                                        return arglist_error("fn", "duplicate-symbol", -1, inner->atom);
                                    }
                                    inner = inner->next;
                                }
                                outer = outer->next;
                            }
                        }
                        
                        return pval_closure(false, x->pval_list->atoms_head->next->atom, arglist, x->pval_list->atoms_head->next->next->next->atom, copy_env(env));

                    }
                }
                else if(strcmp(x->pval_list->atoms_head->atom->pval_symbol, "do") == 0)
                {
                    if (x->pval_list->list_count == 1)
                    {
                        bool b = true;
                        return pval_bool(b);
                    }

                    list_child* temp = x->pval_list->atoms_head->next;
                    while (temp != NULL)
                    {
                        pval* subform = eval_tree(env, temp->atom);
                        // pval_add(temp_list, pval_copy(subform));

                        if (subform->pval_type == PVAL_ERROR)
                        {
                            // pval_delete(temp_list);
                            return subform;
                        }
                        if (temp->next == NULL)
                        {
                            // pval_delete(temp_list);
                            temp = temp->next;
                            return subform;
                        }
                        pval_delete(subform);
                        temp = temp->next;
                    }
                }
                else if(strcmp(x->pval_list->atoms_head->atom->pval_symbol, "loop") == 0)
                {
                    if(x->pval_list->list_count != 2)
                    {
                        return arity_error("loop", "=", 2, x->pval_list->list_count);
                    }
                    while (true)
                    {
                        pval* subform = eval_tree(env, x->pval_list->atoms_head->next->atom);

                        //break out of loop if error
                        if (subform->pval_type == PVAL_ERROR)
                        {
                            return subform;
                        }

                        if (subform->pval_type == PVAL_BOOL && subform->pval_bool == false)
                        {
                            pval_delete(subform);
                            return pval_bool(true);
                        }

                        pval_delete(subform);
                    }
                    
                }
                else if(strcmp(x->pval_list->atoms_head->atom->pval_symbol, "try") == 0)
                {
                    if (x->pval_list->list_count != 2)
                    {
                        return arity_error("try", "=", 2, x->pval_list->list_count);
                    }

                    pval* subform = eval_tree(env, x->pval_list->atoms_head->next->atom);

                    pval* return_list = empty_list();

                    if (subform->pval_type == PVAL_ERROR)
                    {
                        pval_add(return_list, pval_bool(false));
                        pval_add(return_list, pval_copy(subform->pval_error));
                        pval_delete(subform);
                        return return_list;
                    }

                    pval_add(return_list, pval_bool(true));
                    pval_add(return_list, subform);
                    return return_list;
                }
                else if(strcmp(x->pval_list->atoms_head->atom->pval_symbol, "and") == 0)
                {
                    if(x->pval_list->list_count == 1)
                    {
                        return pval_bool(true);
                    }

                    list_child* temp = x->pval_list->atoms_head->next;
                    while (temp != NULL)
                    {
                        pval* subform = eval_tree(env, temp->atom);

                        if (subform->pval_type == PVAL_ERROR)
                        {
                            return subform;
                        }

                        if (subform->pval_type == PVAL_BOOL && subform->pval_bool == false)
                        {
                            return subform;
                        }
                        if (temp->next == NULL)
                        {
                            return subform;
                        }

                        pval_delete(subform);
                        temp = temp->next;
                    }
                }
                else if(strcmp(x->pval_list->atoms_head->atom->pval_symbol, "or") == 0)
                {
                    if(x->pval_list->list_count == 1)
                    {
                        return pval_bool(false);
                    }

                    list_child* temp = x->pval_list->atoms_head->next;
                    while (temp != NULL)
                    {
                        pval* subform = eval_tree(env, temp->atom);

                        if (subform->pval_type == PVAL_ERROR)
                        {
                            return subform;
                        }

                        if (subform->pval_type == PVAL_BOOL && subform->pval_bool != false)
                        {
                            return subform;
                        }
                        if (subform->pval_type != PVAL_BOOL && subform->pval_type != PVAL_ERROR)
                        {
                            return subform;
                        }
                        pval_delete(subform);
                        temp = temp->next;
                    }

                    return pval_bool(false);
                }
                else if(strcmp(x->pval_list->atoms_head->atom->pval_symbol, "let") == 0)
                {
                    if (x->pval_list->list_count == 1)
                    {
                        return arity_error("let", ">=", 1, x->pval_list->list_count);
                    }
                    if ((x->pval_list->list_count-1) % 2 == 0)
                    {
                        char* err_msg = "arity-error: (let) can only accept odd number of arguments.";
                        return pval_error(pval_symbol(err_msg));
                    }

                    // form is (let body) 
                    //base case (let body)
                    if (x->pval_list->list_count == 2)
                    {
                        pval* body = eval_tree(env, x->pval_list->atoms_head->next->atom);
                        return body;
                    }

                    //form is (let name1 expr1 ... nameN exprN body)
                    //recursive cases
                    if (x->pval_list->atoms_head->next->atom->pval_type != PVAL_SYMBOL)
                    {
                        return type_error("let", "symbol", 1, x->pval_list->atoms_head->next->atom);                       
                    }

                    //make -> (fn)
                    pval* fn_list = empty_list();
                    char* fn = "fn";
                    pval_add(fn_list, pval_symbol(fn));

                    //make -> (name)
                    pval* name_list = empty_list();
                    pval* name = x->pval_list->atoms_head->next->atom;
                    pval_add(name_list, pval_copy(name));
                    
                    //make -> (fn (name))
                    pval_add(fn_list, name_list);
                    
                    //make -> (let rest...)
                    pval* inner_let = empty_list();
                    char* let = "let";
                    pval_add(inner_let, pval_symbol(let));
                    
                    list_child* temp = x->pval_list->atoms_head->next->next->next;
                    while (temp != NULL)
                    {
                        pval_add(inner_let, pval_copy(temp->atom));
                        temp = temp->next;
                    }

                    //make -> (fn (name) (let rest))
                    pval_add(fn_list, inner_let);
                    //pull expr
                    pval* expr = x->pval_list->atoms_head->next->next->atom;                    
                    
                    // make -> ()
                    pval* outer = empty_list();

                    //make -> ((fn (name) (let rest)) expr)
                    pval_add(outer, fn_list);
                    pval_add(outer, pval_copy(expr));


                    pval* result = eval_tree(env, outer);
                    pval_delete(outer);
                    return result;
                }
                else if (strcmp(x->pval_list->atoms_head->atom->pval_symbol, "eval") == 0)
                {
                    if (x->pval_list->list_count != 2)
                    {
                        return arity_error("eval", "=", 1, x->pval_list->list_count);
                    }

                    pval* first_eval = eval_tree(env, x->pval_list->atoms_head->next->atom);
                    pval* snd_eval = eval_tree(env, first_eval);

                    pval_delete(first_eval);
                    return snd_eval;

                }
                else if (strcmp(x->pval_list->atoms_head->atom->pval_symbol, "load") == 0)
                {
                    if (x->pval_list->list_count != 2)
                    {
                        return arity_error("load", "=", 1, x->pval_list->list_count);  
                    }
                    pval* eval_filename = eval_tree(env, x->pval_list->atoms_head->next->atom);
                    if (eval_filename->pval_type != PVAL_STRING)
                    {   
                        pval_delete(eval_filename);
                        return type_error("load", "symbol", 1, x->pval_list->atoms_head->next->atom);       
                    }
                    
                    FILE *fptr;
                    fptr = fopen(eval_filename->pval_string->str, "r");
                    
                    if (fptr == NULL)
                    {
                        pval* err = file_error("bad-filename", eval_filename);
                        fclose(fptr);
                        pval_delete(eval_filename);
                        return err;
                    }

                    fseek(fptr, 0L, SEEK_END);
                    int64_t fsize = ftell(fptr);

                    if (fsize < 0)
                    {
                        pval* err = file_error("file-error", eval_filename);
                        fclose(fptr);
                        pval_delete(eval_filename);
                        return err;           
                    }
                    if (fsize == 0)
                    {
                        fclose(fptr);
                        pval_delete(eval_filename);
                        return pval_number(0);                           
                    }
                    pval_delete(eval_filename);

                    rewind(fptr);

                    char* buffer = (char *)malloc((fsize)*sizeof(char));
                    
                    int64_t n = fread(buffer, 1, fsize, fptr);
                    fclose(fptr);

                    pval* buffer_str = pval_string(buffer, n);

                    pval* buffer_list = empty_list();
                    pval_add(buffer_list, buffer_str);
                    
                    Node* parsed_buffer = parse_help(buffer_list);

                    Node* temp = parsed_buffer;

                    int count = 0;
                    while (temp != NULL && terminate == false)
                    {
                        pval* parsed_exp = parse_expression(&temp);
  
                        pval* eval = eval_tree(env, parsed_exp);
                        if (eval->pval_type == PVAL_ERROR)
                        {
                            pval_delete(parsed_exp);
                            free(buffer);
                            deleteList(parsed_buffer);
                            pval_delete(buffer_list);

                            return eval;
                        }
                        count++;
                        pval_delete(parsed_exp);
                        pval_delete(eval);
                    }

                    free(buffer);
                    deleteList(parsed_buffer);
                    pval_delete(buffer_list);

                    return pval_number(count);
                }
                else if (strcmp(x->pval_list->atoms_head->atom->pval_symbol, "macro") == 0)
                {
                    if (x->pval_list->list_count != 4)
                    {
                        return arity_error("macro", "=", 3, x->pval_list->list_count);
                    }

                    if (x->pval_list->atoms_head->next->atom->pval_type != PVAL_SYMBOL)
                    {
                        return type_error("macro", "symbol", 1, x->pval_list->atoms_head->next->atom);
                    }

                    // check that arglist is a list
                    pval* arglist = x->pval_list->atoms_head->next->next->atom;
                    if (arglist->pval_type != PVAL_LIST)
                    {
                        return arglist_error("macro", "not-list", -1, arglist);     
                    }

                    if (arglist->pval_list->atoms_head != NULL)
                    {
                        list_child *temp = arglist->pval_list->atoms_head;

                        //check each element in arg list that it is symbol and that & is second to last
                        int i = 0;
                        while (temp != NULL)
                        {
                            if (temp->atom->pval_type != PVAL_SYMBOL)
                            {
                                return arglist_error("macro", "not-symbol", -1, temp->atom);
                            }
                            if ((strcmp(temp->atom->pval_symbol, "&") == 0) && (i != arglist->pval_list->list_count-2))
                            {
                                if ((strcmp(temp->atom->pval_symbol, "&") == 0) && (i != arglist->pval_list->list_count-2))
                                {
                                    return arglist_error("fn", "bad-&-position", i, temp->atom);
                                }
                            }
                            i++;
                            temp = temp->next;
                        }
                        //check for duplicate symbols in arglist
                        list_child* outer = arglist->pval_list->atoms_head;

                        while (outer->next != NULL)
                        {
                            list_child *inner = outer->next;
                            while (inner != NULL)
                            {
                                if (strcmp(outer->atom->pval_symbol, inner->atom->pval_symbol) == 0)
                                {
                                    return arglist_error("macro", "duplicate-symbol", -1, inner->atom);
                                }
                                inner = inner->next;
                            }
                            outer = outer->next;
                        }
                    }
                    
                    pval* macro = pval_closure(true, x->pval_list->atoms_head->next->atom, arglist, x->pval_list->atoms_head->next->next->next->atom, copy_env(env));
                
                    env_add_binding(env, create_binding(x->pval_list->atoms_head->next->atom->pval_symbol, pval_copy(macro)));
                    return macro;
                }
            }

            pval *evaluate = eval_tree(env, x->pval_list->atoms_head->atom);
            // built in functions here
            if (evaluate->pval_type == PVAL_FUNCTION)
            {
                pval* temp_list = empty_list();

                list_child* temp = x->pval_list->atoms_head->next;

                while(temp != NULL)
                {
                    pval* result = eval_tree(env, temp->atom);
                    if (result->pval_type == PVAL_ERROR) { 
                        
                        pval_delete(evaluate);
                        pval_delete(temp_list);
                        return result;
                    }
                    pval_add(temp_list, result);
                    temp = temp->next;
                }

                pval* temp_val = evaluate->pval_func->pval_fn_def(temp_list);

                pval_delete(evaluate);
                pval_delete(temp_list);
                
                return temp_val;
            }
            //user defined functions here
            //head has already been evaluted
            //taking closure path here
            else if (evaluate->pval_type == PVAL_CLOSURE)
            {
                // evaluate the arguments here left to right, propagating left most error
                pval* temp_list = empty_list();

                list_child* temp = x->pval_list->atoms_head->next;

                while(temp != NULL)
                {
                    //if the closure is a macro, then pass in the unevaluated subforms to the temp_list
                    // otherwise as a function, evalaute the arguments then pass into temp_list
                    if (evaluate->pval_closure->is_macro)
                    {
                        pval_add(temp_list, pval_copy(temp->atom));
                    }
                    else
                    {
                        pval* result = eval_tree(env, temp->atom);
                        if (result->pval_type == PVAL_ERROR) { 
                            
                            pval_delete(evaluate);
                            pval_delete(temp_list);
                            return result;
                        }
                        pval_add(temp_list, result);
                    }
                    temp = temp->next;
                }
                
                //arity check here
                list_child* check_args = evaluate->pval_closure->arglist->pval_list->atoms_head;
                bool has_ampersand = false;

                while (check_args != NULL)
                {
                    if (strcmp(check_args->atom->pval_symbol, "&") == 0)
                    {
                        has_ampersand = true;
                        check_args = NULL;
                        break;
                    }
                    check_args = check_args->next;
                }
                
                //checks if & is in the second to last position
                if (has_ampersand == false)
                {
                    if (evaluate->pval_closure->arglist->pval_list->list_count != temp_list->pval_list->list_count)
                    {
                        pval_delete(temp_list);
                        pval_delete(evaluate);
                        char *err_msg = "error: arity-error (evaluating fn) got different number of arguments than expected.";
                        return pval_error(pval_symbol(err_msg));
                    }
                }

                //checks if the arglist count is >= 2
                if (temp_list->pval_list->list_count < evaluate->pval_closure->arglist->pval_list->list_count - 2)
                {
                    pval_delete(temp_list);
                    pval_delete(evaluate);
                    char *err_msg = "error: arity-error (evaluating fn) got < 2 arguments when expecting at least 2.";
                    return pval_error(pval_symbol(err_msg));
                }

                //create a new environment (or frame)
                //the parent environment is the closure's frozen env
                environment* new_frame = create_env();
                add_env_parent(new_frame, evaluate->pval_closure->env);

                //bind the parameters of the function here
                temp = temp_list->pval_list->atoms_head;
   
                list_child* temp_arg = evaluate->pval_closure->arglist->pval_list->atoms_head;
                while (temp_arg != NULL)
                {
                    if (strcmp(temp_arg->atom->pval_symbol, "&") == 0)
                    {
                        temp_arg = temp_arg->next;
                        pval* new_list = empty_list();

                        while (temp != NULL)
                        {
                            pval_add(new_list, pval_copy(temp->atom));
                            temp = temp->next;
                        }
                        env_add_binding(new_frame, create_binding(temp_arg->atom->pval_symbol, pval_copy(new_list)));
                        pval_delete(new_list);
                        temp_arg = temp_arg->next;
                    }
                    else
                    {
                        env_add_binding(new_frame, create_binding(temp_arg->atom->pval_symbol, pval_copy(temp->atom)));
                        temp_arg = temp_arg->next;
                        temp = temp->next;
                    }
                }
                
                //if the function is named, then bind the function closure (as a pval) to the newly created environment
                if (evaluate->pval_closure->fn_name != NULL)
                {
                    env_add_binding(new_frame, create_binding(evaluate->pval_closure->fn_name->pval_symbol, pval_copy(evaluate)));
                }

                //evaluate the function body in this new frame
                pval* result = eval_tree(new_frame, evaluate->pval_closure->body);

                //delete the frame after body is evaluated
                delete_env(new_frame);

                //this will evaluate the macro in the current caller's env, the middle portion of the code will evaluate the macroexpansion
                // now you need to evaluate the expanded macro with the caller's arugments
                if (evaluate->pval_closure->is_macro)
                {
                    pval* eval_exp = eval_tree(env, result);

                    pval_delete(evaluate);
                    pval_delete(temp_list);
                    pval_delete(result);

                    return eval_exp;
                }
                

                pval_delete(evaluate);
                pval_delete(temp_list);
                
                return result;
            }
            else if (evaluate->pval_type == PVAL_ERROR)
            {
                return evaluate;
            }
            pval_delete(evaluate);
            return pval_error(pval_symbol("inapplicable-head"));
        }
    }
    else {
        pval* err_list = empty_list();
        pval_add(err_list, pval_symbol("evaluation-error"));
        pval_add(err_list, pval_symbol("eval_tree"));
        pval_add(err_list, pval_symbol("undefined-type"));
        pval_add(err_list, pval_number(x->pval_type));

        return pval_error(err_list);
    }
};
