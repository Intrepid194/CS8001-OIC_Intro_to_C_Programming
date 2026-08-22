#ifndef LEXER
#define LEXER

typedef enum {
    INVALID_TOKEN,
    INCOMPLETE_PARSE,
    NO_ERROR
} node_type;

typedef struct Node {
    node_type error;
    char *token;
    struct Node *next;
} Node;

void printList(Node *node);

void deleteList(Node *head);

char *createToken(char command);

// Node *createNode(char *token);
Node *createNode(char* token, node_type error);

// void append(Node **head, char *token);
void append(Node** head, char *token, node_type error);

Node *lex(Node *head, char *command);

#endif