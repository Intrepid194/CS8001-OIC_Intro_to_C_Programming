#include "../include/lexer.h"
#include "../include/parser.h"

//data structures
typedef struct builtin {
    char* name;
    pval* pval;
} builtin;

//helper and evaluate functions
pval* pval_copy(pval* pv);

pval* check_type(list_child* x);

pval* lookup_builtin(char* builtin_fn_name);

pval* eval_tree(pval* x);

// built in functions
pval* add(pval* list);

pval* subtract(pval* list);

pval* subtract(pval* list);

pval* divide(pval* list);

pval* equals(pval* list);

pval* exit_prg(pval* list);


