#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <ctype.h>

#include "../include/parser.h"
#include "../include/eval.h"
#include "../include/pvals.h"

//grammar rule expression -> atom | list
pval* parse_expression(Node** current) 
{
    if ((*current)->error == INVALID_TOKEN)
    {
        pval* err_list = empty_list();
        pval_add(err_list, pval_symbol("invalid-token"));
        pval_add(err_list, pval_symbol((*current)->token));
        
        (*current) =(*current)->next;

        return pval_error(err_list);
    }
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
    else if (strcmp((*current)->token, "'") == 0)
    {
        //this handles the short hand ' for quote outliend in phase 2
        
        pval* quote_list = empty_list();
        
        char* quote = "quote";
        pval_add(quote_list, pval_symbol(quote));

        if((*current)->next == NULL)
        {
            pval_delete(quote_list);
            (*current) = (*current)->next;
            char *err_msg = "error: ' incomplete parse.";
            return pval_error(pval_symbol(err_msg));
        }
        (*current) = (*current)->next;
        pval_add(quote_list, parse_expression(current));

        return quote_list;
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
    
    if ((*current) == NULL)
    {
        pval_delete(new_list);
        return pval_error(pval_symbol("incomplete-parse"));
    }

    (*current) = (*current)->next;

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

        if (strcmp((*current)->token, "#t") == 0)
        {
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

        temp = strtoll((*current)->token, NULL, 10);

        pval* number = pval_number(temp);

        (*current) = (*current)->next;
        return number;
    }
    //parse strings
    // atom -> string
    else if ((*current)->token[0] == '\"')
    {
        char *temp_str = (char *)malloc(strlen((*current)->token)*sizeof(char));
        int pval_len = 0;
        int num_par = 0;
        for (int j=0; ((*current)->token[j] != '\0'); j++)
        {
            // edge case #1 : check for \xZW (hexadecimal strings) 0-255 
            if( ((*current)->token[j] == '\\') && 
                ((*current)->token[j+1] == 'x') && 
                (isxdigit((*current)->token[j+2])) && 
                (isxdigit((*current)->token[j+3]))
            )
            {
                unsigned int temp;
                sscanf(&((*current)->token[j+2]), "%2x", &temp);

                temp_str[pval_len] = (unsigned char)temp;
                pval_len++; 
                j+=3;
            }
            //edge case #2: check for other escaped characters \n \0 \" '\\'
            else if (((*current)->token[j] == '\\') && ((*current)->token[j+1] == 'n') 
            )
            {
                temp_str[pval_len] = '\n';
                pval_len++;
                j++;
            }
            else if (((*current)->token[j] == '\\') && ((*current)->token[j+1] == '0') 
            )
            {
                temp_str[pval_len] = '\0';
                pval_len++;
                j++;
            }
            else if (((*current)->token[j] == '\\') && ((*current)->token[j+1] == '\"') 
            )
            {
                temp_str[pval_len] = '\"';
                pval_len++;
                j++;
            }
            else if (((*current)->token[j] == '\\') && ((*current)->token[j+1] == '\\') 
            )
            {
                temp_str[pval_len] = '\\';
                pval_len++;
                j++;
            }
            else if ((*current)->token[j] == '\"')
            {
                num_par++;
            } 
            //get the rest of the characters
            else
            {
                temp_str[pval_len] = (*current)->token[j];
                pval_len++;
            }
            
        }

        if (num_par != 2)
        {
            char* err_msg = "error: incomplete parse";
            free(temp_str);
            (*current) = (*current)->next;
            return pval_error(pval_symbol(err_msg));
        }

        (*current) = (*current)->next;
        pval* pv_str = pval_string(temp_str, pval_len);
        free(temp_str);
        return pv_str;

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