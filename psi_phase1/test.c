#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct Node {
    char *token;
    struct Node *next;
} Node;

Node *createNode(char *temp);

int main () {


    char *temp = "(+ 1 2)";

    Node *head = NULL;
    Node *newNode = createNode(temp);

    // Node *newNode = (Node *)malloc(sizeof(Node));
    // newNode->token = temp;
    // newNode->next = NULL;
    printf("%s", newNode->token);
    
    //createNde("r", sizeof("r"));

    return 0;
}

Node *createNode(char *temp) { 

    Node *newNode = (Node *)malloc(sizeof(Node));

    // newNode->token = (char *)(malloc(sizeof(char) * n));
    newNode->token = temp;
    newNode->next = NULL;

    return newNode;
}