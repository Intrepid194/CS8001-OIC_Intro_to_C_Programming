
#include <stdint.h>
#include <stdbool.h>

#include "../include/lexer.h"

#ifndef PARSER
#define PARSER

typedef enum {
    PVAL_NUMBER,
    PVAL_BOOL,
    PVAL_SYMBOL,
    PVAL_LIST,
    PVAL_FUNCTION,
    PVAL_ERROR
} pval_tag;



typedef struct pval pval;

typedef pval* (*pval_function)(pval*);

typedef struct pv_func pv_func;

struct pv_func {
    char* func_name;
    pval_function pval_fn_def;
};

struct pval {
    pval_tag pval_type;
    union {
        int64_t pval_number;
        bool pval_bool;
        char *pval_symbol;
        struct list *pval_list;
        struct pv_func *pval_func;
        struct pval *pval_error;
    };
};

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

pval* pval_func(pval_function pval_fn, char* func_name);

pval* pval_number(int64_t n);

pval* pval_bool(bool b);

pval* pval_error(pval* x);

pval* pval_symbol(char *symbol);

pval* empty_list();

void pval_add(pval* list, pval* elem);

void pval_print(pval* pv);

void pval_delete(pval* pv);

pval* parse_atom(Node** current);

pval* parse_list(Node** current);

pval* parse_expression(Node** current);

#endif