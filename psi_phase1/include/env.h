#include "../include/pvals.h"


#ifndef ENV
#define ENV
//data structures
typedef struct binding binding;
struct binding {
    char* name;
    pval* value;
    struct binding* next;
};

struct environment {
    binding *bindings; //bindings for this local environment
    struct environment *parent; // search parent environment if search doesn't find aynthing in this local environment
};

bool check_protected_symbols(char* name);

typedef struct closure closure;

struct closure {
    bool is_macro;
    pval* fn_name; //a pval_symbol
    pval* arglist;
    pval* body;
    environment* env;
};

// global/local environment functions
environment* create_env();

void add_env_parent(environment* env, environment* parent);

binding* create_binding(char* name, pval* value);

void env_add_binding(environment* env, binding* func);

pval* lookup_env_binding(environment* env, char* binding_name);

void delete_env(environment* env);

void delete_env_chain(environment* env);

environment* copy_env(environment *env);

#endif