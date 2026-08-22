#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <ctype.h>

#include "../include/env.h"

void delete_env(environment* env)
{
    //base case, if env is NULL then return
    if (env == NULL) { return; }
    //delete local bindings
    binding *head = env->bindings;

    while (head != NULL)
    {
        binding *temp = head;
        char *temp_name = head->name;
        pval *temp_value = head->value;

        pval_delete(temp_value);
        free(temp_name);

        head = head->next;
        free(temp);
    }
    free(env);
}

void delete_env_chain(environment* env)
{
    //base if the parent is NULL then return
    if (env == NULL) { return; };
    //grab env's parent
    environment* temp_parent = env->parent;

    //delete local bindings and the env
    delete_env(env);

    //walk the chain into the parent too
    delete_env_chain(temp_parent);
}

environment* create_env() 
{
    environment* env = (environment *)malloc(sizeof(environment));

    env->bindings = NULL;
    env->parent = NULL;

    return env;
}

environment* copy_env(environment *env)
{
    //base case: if parent is NULL then return new_env as NULL
    if (env == NULL) { return NULL; } 
    //create new env copy
    environment *new_env = create_env();
    

    //add the local bindings to env_env copy
    binding* head = env->bindings;
    while (head != NULL)
    {
        env_add_binding(new_env, create_binding(head->name, pval_copy(head->value)));
        head = head->next;
    }

    //else: recursive copy env->parent all the way up until env->parent is NULL
    //then return new_env that is copied
    new_env->parent = copy_env(env->parent);
    return new_env;
}

void add_env_parent(environment* env, environment* parent)
{
    if (env->parent == NULL)
    {
        env->parent = parent;
        return;
    }

    return;
}

binding* create_binding(char* name, pval* value)
{
    binding* new_binding = (binding *)malloc(sizeof(binding));

    new_binding->name = strdup(name);
    new_binding->value = value;
    new_binding->next = NULL;

    return new_binding;
}

void env_add_binding(environment* env, binding* func)
{
    //if no bindings present, add to the env as first binding
    if (env->bindings == NULL)
    {
        env->bindings = func;
        return;
    }
    
    // override binding if it is found in the current env.
    binding* temp = env->bindings;

    while (temp != NULL)
    {
        if (strcmp(temp->name, func->name) == 0)
        {
            pval* temp_val = temp->value;

            temp->value = func->value;

            pval_delete(temp_val);

            char* func_name = func->name;
            free(func_name);
            free(func);
            
            return;
        }
        temp = temp->next;
    }

    //add binding to the end if not found
    temp = env->bindings;
    while(temp->next != NULL)
    {
        temp = temp->next;
    }
    temp->next = func;
    return;
}


pval* lookup_env_binding(environment* env, char* binding_name)
{   
    //base case first which is search the bindings in current local env
    binding* temp = env->bindings;
    while (temp != NULL)
    {
        if (strcmp(temp->name, binding_name) == 0)
        {
            return pval_copy(temp->value);
        }
        temp = temp->next;
    }
    //recursive case search the parent env for the binding
    if (env->parent != NULL) 
    {
        return lookup_env_binding(env->parent, binding_name);
    }
    
    //err_msg if bindings cannot be found in the parent or the local env
    pval* err_list = empty_list();
    pval_add(err_list, pval_symbol("unbound"));
    pval_add(err_list, pval_symbol(binding_name));
    return pval_error(err_list);
}