#ifndef LEXER
#define LEXER

typedef struct Node {
    char *token;
    struct Node *next;
} Node;

void printList(Node *node);

void deleteList(Node *head);

char *createToken(char command);

Node *createNode(char *token);

void append(Node **head, char *token);

Node *lex(Node *head, char *command);

#endif