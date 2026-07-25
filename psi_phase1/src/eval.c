#include <string.h>

#include "../include/lexer.h"
#include "../include/parser.h"
#include "../include/eval.h"
#include "../include/globals.h"

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
    else if (pv->pval_type == PVAL_FUNCTION)
    {
        return pval_func(pv->pval_func->pval_fn_def, pv->pval_func->func_name);
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
        char *err_msg = "error: Unable to copy current pval...";
        return pval_error(pval_symbol(err_msg));
    }
};

pval* check_type(list_child* x)
{
    if (x->atom->pval_type == PVAL_BOOL)
    {
        char* err_msg = "$type error: got a bool when expecting a number...";
        return pval_error(pval_symbol(err_msg));
    }
    else if (x->atom->pval_type == PVAL_SYMBOL)
    {
        char* err_msg = "$type error: got a symbol when expecting a number...";
        return pval_error(pval_symbol(err_msg));
    }
    else if (x->atom->pval_type == PVAL_LIST)
    {
        char* err_msg = "$type error: got a list when expecting a number...";
        return pval_error(pval_symbol(err_msg));
    }
    else if(x->atom->pval_type == PVAL_FUNCTION)
    {
        char* err_msg = "$type error: got a function when expecting a number...";
        return pval_error(pval_symbol(err_msg));
    }
    else
    {
        char* err_msg = "$type error: got a non-number when expecting a number...";
        return pval_error(pval_symbol(err_msg));
    }
}

pval* add(pval* list)
{
    list_child* temp = list->pval_list->atoms_head;

    int64_t sum = 0;

    while(temp != NULL)
    {
        if (temp->atom->pval_type == PVAL_NUMBER)
        {
            sum = sum + temp->atom->pval_number;
        }
        else
        {
            return check_type(temp);
        }
        temp = temp->next;
    }

    return pval_number(sum);
}

pval* subtract(pval* list)
{
    list_child* temp = list->pval_list->atoms_head;

    //checks for arity 0
    if (temp == NULL)
    {   
        char* err_msg = "$error: requires more than 0 args...";
        return pval_error(pval_symbol(err_msg));
    }

    int64_t difference = 0;
    int64_t count = 0;

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
            return check_type(temp);
        }
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

    while (temp != NULL)
    {   
        if (temp->atom->pval_type == PVAL_NUMBER)
        {
            product = product * temp->atom->pval_number;
        }
        else{
            return check_type(temp);
        }
        temp = temp->next;        
    }

    return pval_number(product);
};

pval* divide(pval* list)
{
    list_child* temp = list->pval_list->atoms_head;

    int64_t quotient = 0;
    int64_t count = 0;

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
                    char* err_msg = "error: cannot divide by 0!";
                    return pval_error(pval_symbol(err_msg));
                }
                else
                {
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
            return check_type(temp);
        }
        count++;
        temp = temp->next;
        
    }
    if (count < 2)
    {
        char* err_msg = "error: division requires at least 2 arguments...";
        return pval_error(pval_symbol(err_msg));
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
        else{
            b = false;
            return pval_bool(b);
        }
    }
    return pval_bool(b);
};

pval* exit_prg(pval* list)
{
    list_child* temp = list->pval_list->atoms_head;

    if (temp != NULL)
    {
        char* err_msg = "$error: arity-error, quit cannot have > 0 arguments...";
        return pval_error(pval_symbol(err_msg));

    }
    terminate = true;
    return pval_number(0);
};

pval* lookup_builtin(char* builtin_fn_name)
{
    if (strcmp(builtin_fn_name, "+") == 0) { return pval_func(add, "+"); };
    if (strcmp(builtin_fn_name, "-") == 0) { return pval_func(subtract, "-"); };
    if (strcmp(builtin_fn_name, "*") == 0) { return pval_func(multiply, "*"); };
    if (strcmp(builtin_fn_name, "/") == 0) { return pval_func(divide, "/"); };
    if (strcmp(builtin_fn_name, "=") == 0) { return pval_func(equals, "="); };
    if (strcmp(builtin_fn_name, "quit") == 0) { return pval_func(exit_prg, "quit"); };

    char *err_msg = "Could not find built-in function for symbol...";
    return pval_error(pval_symbol(err_msg));
};

pval* eval_tree(pval* x)
{
    if (x->pval_type == PVAL_BOOL || x->pval_type == PVAL_NUMBER || x->pval_type == PVAL_ERROR || x->pval_type == PVAL_FUNCTION)
    {
        return pval_copy(x);
    }
    else if (x->pval_type == PVAL_SYMBOL)
    {   
        return lookup_builtin(x->pval_symbol);
    }
    else if (x->pval_type == PVAL_LIST)
    {
        if (x->pval_list->atoms_head == NULL)
        {
            return pval_copy(x);
        }
        else
        {
            pval *evaluate = eval_tree(x->pval_list->atoms_head->atom);
            if (evaluate->pval_type == PVAL_FUNCTION)
            {
                pval* temp_list = empty_list();

                list_child* temp = x->pval_list->atoms_head->next;

                while(temp != NULL)
                {
                    pval* result = eval_tree(temp->atom);
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
            pval_delete(evaluate);
            char *err_msg = "error: inapplicable head";
            return pval_error(pval_symbol(err_msg));
        }
    }
    else {
        char *err_msg = "error: cannot evalute tree in current form...";
        return pval_error(pval_symbol(err_msg));
    }
};
