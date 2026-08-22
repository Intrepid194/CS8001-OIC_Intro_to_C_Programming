#include "../include/lexer.h"
#include "../include/parser.h"
#include "../include/env.h"
//helper and evaluate functions

#ifndef EVAL
#define EVAL

pval* check_type(list_child* x);

pval* eval_tree(environment* env, pval* x);

pval* arity_error(char* func, char* cmp_symbol, int64_t exp_args, int64_t actual_args);

pval* arity_error_btwn_2_args(char* func, char* cmp_symbol, int64_t actual_args);

pval* type_error(char* func, char* exp_type, int64_t position, pval* value);

pval* value_error(char* func, pval* value);

// built in functions
pval* add(pval* list);

pval* subtract(pval* list);

pval* multiply(pval* list);

pval* divide(pval* list);

pval* equals(pval* list);

pval* gt(pval* list);

pval* lt(pval* list);

pval* gte(pval* list);

pval* lte(pval* list);

pval* not_eq(pval* list);

pval* modulo(pval* list);

pval* head(pval* list);

pval* tail(pval* list);

pval* cons(pval* list);

pval* not(pval* list);

pval* type(pval* x);

pval* exit_prg(pval* list);

pval* ord(pval* list);

pval* str_to_chr(pval* list);

pval* input(pval* str);

pval* output(pval* list);

pval* create_cell(pval* list);

pval* deref_cell(pval* list);

pval* write_cell(pval* list);

pval* str(pval* list);

pval* print(pval* list);

pval* error(pval* list);

Node* parse_help(pval* list);

pval* read(pval* list);

pval* make_list(pval* list);

pval* get_file(pval* list);

pval* put_file(pval* list);

pval* new_symbol(pval* list);

#endif