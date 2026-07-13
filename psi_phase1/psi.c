#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
// Lexer Function prototypes
typedef struct Node {
    char *token;
    struct Node *next;
} Node;

void printList(Node *node);

void deleteList(Node *head);

char *createToken(char command);

Node *createNode(char *token);

void append(Node **head, char *token);

//Parser Prototypes

typedef enum {
    PVAL_NUMBER,
    PVAL_BOOL,
    PVAL_SYMBOL,
    PVAL_LIST,
    PVAL_FUNCTION,
    PVAL_ERROR
} pval_tag;

typedef struct pval pval;

typedef struct pval {
    pval_tag pval_type;
    union {
        int64_t pval_number;
        bool pval_bool;
        char *pval_symbol;
        struct list *pval_list;
        struct function *pval_function;
        struct pval *pval_error;
    };
} pval;

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

pval* pval_number(int64_t n);

pval* pval_bool(bool b);

pval* pval_error(pval* x);

pval* pval_symbol(char *symbol);

pval* empty_list();

void pval_add(pval* list, pval* elem);

void pval_print(pval* pv);

pval* pval_copy(pval* pv);

void pval_delete(pval* pv);

int main() {

    while (1) {
        Node *head = NULL;

        char *command = malloc(sizeof(char)*4097);
        printf("psi>");

        if (fgets(command, 4097, stdin) == NULL) {
            printf("No meaningful input found. Exiting program...");
            free(command);
            break;
        }
        else 
        {
            for (int i=0; command[i] != '\0'; i++) {

                char c = command[i];
                char next_c = command[i+1];

                if (c == '(' || c == ')') 
                {
                    append(&head, createToken(c));
                } 
                else if (c == ' ' || c == '\r' || c == '\n') 
                {
                    continue;
                } 
                else if ((c == '+' && (next_c >= 48 && next_c <= 57)) || (c == '-' && (next_c >= 48 && next_c <= 57)))
                {
                    char *temp = (char *)calloc(4097, sizeof(char));
                    int temp_idx = 1;

                    temp[0] = command[i];

                    for (int j = i+1; (command[j] != ')') && (command[j] != '\0') && (command[j] != ' '); j++)
                    {
                        if (command[j] >= 48 && command[j] <= 57)
                        {
                            temp[temp_idx] = command[j];
                            temp_idx++;
                        }
                        i = j;
                    }
                    append(&head, temp);
                }
                else if (c == '*' || c == '/' || c == '+' || c == '-' || c == '=') 
                {
                    append(&head, createToken(c));
                }
                else if (c == '#' && (next_c == 'f' || next_c == 't'))
                {
                    char *temp = (char *)calloc(3, sizeof(char));

                    temp[0] = command[i];
                    temp[1] = command[i+1];
                    temp[2] = '\0';

                    append(&head, temp);

                    i++;
                }
                else if (c >= 48 && c <= 57)
                {
                        if (command[i+1] == ' ' || command[i+1] == ')') 
                        {
                            append(&head, createToken(command[i]));
                        } 
                        else 
                        {
                            char *temp = (char *)calloc(4097, sizeof(char));
                            int temp_idx = 0;

                            for (int j = i; (command[j] != ')') && (command[j] != '\0') && (command[j] != ' '); j++) 
                            {
                                if (command[j] >= 48 && command[j] <= 57) {
                                    temp[temp_idx] = command[j];
                                    temp_idx++;
                                }
                                i = j;
                            } 

                            append(&head, temp);
                        }
                }
                else if ((c >= 65 && c <= 90) || (c >= 97 && c <= 122))
                { 
                    char *temp = (char *)calloc(4097, sizeof(char));
                    int temp_idx = 0;

                    for (int j = i; (command[j] != ')') && (command[j] != '\0') && (command[j] != ' '); j++)
                    {
                        if ((command[j] >= 65 && command[j] <= 90) || (command[j] >= 97 && command[j] <= 122))
                        {
                            temp[temp_idx] = command[j];
                            temp_idx++;
                        }
                        i = j;
                    }

                    append(&head, temp);
                }
                else {
                    printf("Invalid token error");
                }
                
            }

            free(command);
        }

        // printList(head);
        deleteList(head);
    }


    return 0;
}

// Lexer function defs
char *createToken(char command) {

    char *token = malloc(sizeof(char)*2);
    token[0] = command;
    token[1] = '\0';

    return token;
}

Node *createNode(char *token) { 

    Node *newNode = (Node *)malloc(sizeof(Node));

    newNode->token = token;
    newNode->next = NULL;

    return newNode;
}

void append(Node** head, char *token) {

    Node *new_node = createNode(token);

    Node *last = *head;

    if (*head == NULL) {

        *head = new_node;
        return;
    }

    while (last->next != NULL) {
        last = last->next;
    }

    last->next = new_node;
    return;
}

void printList(Node *node)
{
  while (node != NULL)
  {
     printf(" %s ", node->token);
     node = node->next;
  }
}

void deleteList(Node *head)
{
    if (head == NULL) {
        return;
    }
    
    while (head != NULL) {
        Node *temp = head;
        char *temp_str = temp->token;

        free(temp_str);

        head = head->next;
        free(temp);
    }
}

//Parser function def
//pval constructor functions
pval* pval_number(int64_t n) 
{
    pval *pval_num = (pval *)malloc(sizeof(pval));

    pval_num->pval_type = PVAL_NUMBER;
    pval_num->pval_number = n;

    return pval_num;
};

pval* pval_bool(bool b) 
{
    pval* pval_bl = (pval *)malloc(sizeof(pval));

    pval_bl->pval_type = PVAL_BOOL;
    pval_bl->pval_bool = b;
    
    return pval_bl;
};

pval* pval_symbol(char *symbol)
{
    pval* pval_sym = (pval *)malloc(sizeof(pval));

    pval_sym->pval_type = PVAL_SYMBOL;

    //duplicate the symbol tokens in the tokenized list so the token list can be freed after it is parsed.
    //need to free this symbol when defining pval_delete().
    pval_sym->pval_symbol = strdup(symbol);

    return pval_sym;
};

pval* pval_list() 
{

};

pval* pval_function() {};

pval* pval_error(pval* x) {};

//makes empty list to add pvals to later by calling pval_add().
pval* empty_list()
{
    pval *em_list = (pval *)malloc(sizeof(pval));

    em_list->pval_type = PVAL_LIST;

    list *new_list = (list *)malloc(sizeof(list));
    new_list->list_count = 0;
    new_list->atoms_head = NULL;

    em_list->pval_list = new_list;

    return em_list;
};

//adds a pval element to the list passed into it.
void pval_add(pval* list, pval* elem)
{

};

void pval_print(pval* pv) {};

pval* pval_copy(pval* pv) {};

void pval_delete(pval* pv) {};