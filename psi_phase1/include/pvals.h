#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <ctype.h>

#ifndef PVALS
#define PVALS

typedef struct environment environment;

typedef enum {
    PVAL_NUMBER,
    PVAL_BOOL,
    PVAL_SYMBOL,
    PVAL_LIST,
    PVAL_FUNCTION,
    PVAL_ERROR,
    PVAL_STRING,
    PVAL_CLOSURE,
    PVAL_CELL
} pval_tag;

typedef struct pval pval;

typedef pval* (*pval_function)(pval*);

typedef struct pv_func pv_func;

struct pv_func {
    char* func_name;
    pval_function pval_fn_def;
};

typedef struct pv_str pv_str;

struct pv_str {
    int str_len;
    char *str;
};

struct pval {
    pval_tag pval_type;
    int64_t reference_count;
    union {
        struct closure *pval_closure;
        int64_t pval_number;
        bool pval_bool;
        struct pv_str *pval_string;
        char *pval_symbol;
        struct list *pval_list;
        struct pv_func *pval_func;
        struct pval *pval_error;
        struct pval *pval_cell;
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

pval* pval_closure(bool is_macro, pval* fn_name, pval* arglist, pval* body, environment* env);

pval* pval_func(pval_function pval_fn, char* func_name);

pval* pval_number(int64_t n);

pval* pval_bool(bool b);

pval* pval_error(pval* x);

pval* pval_cell(pval* x);

pval* pval_symbol(char *symbol);

pval* pval_string(char *str, int str_len);

pval* empty_list();

pval* pval_copy(pval* pv);

void pval_add(pval* list, pval* elem);

void pval_print(pval* pv);

void pval_delete(pval* pv);

#endif