#include "../include/lexer.h"
#include "../include/pvals.h"


#ifndef PARSER
#define PARSER

pval* parse_atom(Node** current);

pval* parse_list(Node** current);

pval* parse_expression(Node** current);

#endif