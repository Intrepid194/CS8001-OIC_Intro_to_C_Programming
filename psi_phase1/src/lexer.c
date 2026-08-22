#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <ctype.h>

#include "../include/lexer.h"


char *createToken(char command) {

    char *token = malloc(sizeof(char)*2);
    token[0] = command;
    token[1] = '\0';

    return token;
}

// Node *createNode(char *token) { 

//     Node *newNode = (Node *)malloc(sizeof(Node));

//     newNode->token = token;
//     newNode->next = NULL;

//     return newNode;
// }

Node *createNode(char* token, node_type error) 
{ 

    Node *newNode = (Node *)malloc(sizeof(Node));

    newNode->error = error;
    newNode->token = token;
    newNode->next = NULL;

    return newNode;
}

// void append(Node** head, Node *new_node) 
// {

//     Node *last = *head;

//     if (*head == NULL) {

//         *head = new_node;
//         return;
//     }

//     while (last->next != NULL) {
//         last = last->next;
//     }

//     last->next = new_node;
//     return;
// }
void append(Node** head, char *token, node_type error) {

    Node *new_node = createNode(token, error);

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

// void append(Node** head, char *token) {

//     Node *new_node = createNode(token);

//     Node *last = *head;

//     if (*head == NULL) {

//         *head = new_node;
//         return;
//     }

//     while (last->next != NULL) {
//         last = last->next;
//     }

//     last->next = new_node;
//     return;
// }

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

Node *lex(Node *head, char* command) {
    int command_len = strlen(command);

    if (command_len > 0 && command[command_len-1] == '\n') 
    { 
        command[command_len-1] = '\0'; 
        command_len--;
    }
    
    if (command_len > 0 && command[command_len-1] == '\r')
     { 
        command[command_len-1] = '\0'; 
        command_len--;
    }

    for (int i=0; command[i] != '\0'; i++) {

        char c = command[i];
        char next_c = command[i+1];
        
        //tokenize '(' and ')'
        if (c == '(' || c == ')')
        {
            append(&head, createToken(c), NO_ERROR);
        }
        // tokenize ';' and remaining line (comment strings)
        else if (c == ';')
        {
            for (int j = i; (command[j] != '\0') && command[j] != '\n'; j++)
            {
                i = j;
            }
        }
        //tokenize booleans '#t' and '#f'
        else if (c == '#' && (next_c == 'f' || next_c == 't'))
        {
            char *temp = (char *)calloc(3, sizeof(char));

            temp[0] = command[i];
            temp[1] = command[i+1];
            temp[2] = '\0';

            append(&head, temp, NO_ERROR);
            i++;
        }
        //tokenize -0/+0 thru -9/+9
        else if ((c == '+' && (next_c >= '0' && next_c <= '9')) || (c == '-' && (next_c >= '0' && next_c <= '9')))
        {
            char *temp = (char *)calloc(4097, sizeof(char));
            int err_flag = 0;
            int temp_idx = 1;

            temp[0] = command[i];

            for (int j = i+1; strchr("\n\r\t\"\'; ()", command[j]) == NULL && command[j] != '\0'; j++)
            {
                if (command[j] >= '0' && command[j] <= '9')
                {
                    temp[temp_idx] = command[j];
                    temp_idx++;
                }
                else{
                    err_flag = 1;
                }
                i = j;
            }
            if (err_flag == 0)
            {
                append(&head, temp, NO_ERROR);
            }
            else{
                append(&head, temp, INVALID_TOKEN);
            }
            
        }
        //check if symbol is "0-9" then follow by not that
        else if (strchr("0123456789", c ) != NULL && strchr("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ+-*/%=<>!:", next_c) != NULL && next_c != '\0')
        {
            char *temp = (char *)calloc(4097, sizeof(char));
            int temp_idx = 0;

            for (int j = i; strchr("\n\r\t\"\'; ()", command[j]) == NULL && command[j] != '\0'; j++)
            {
                temp[temp_idx] = command[j];
                temp_idx++;

                i = j;
            }
            append(&head, temp, INVALID_TOKEN);
        }
        //tokenize 0 thru 9
        else if (c >= '0' && c <= '9')
        {
            char *temp = (char *)calloc(4097, sizeof(char));
            int temp_idx = 0;
            int err_flag = 0;

            for (int j = i; strchr("\n\r\t\"\'; ()", command[j]) == NULL && command[j] != '\0'; j++) 
            {
                if (command[j] >= '0' && command[j] <= '9') {
                    temp[temp_idx] = command[j];
                    temp_idx++;
                }
                else
                {
                    err_flag = 1;
                }
                i = j;
            } 
            if (err_flag == 0)
            {
                append(&head, temp, NO_ERROR);
            }
            else{
                append(&head, temp, INVALID_TOKEN);
            }
        }
        //tokenize strings anything like "string"
        else if (c == '\"')
        {
            //gets string length
            int len = 0;
            for (int j = i+1; (command[j] != '\0') && (command[j] != '\"'); j++) 
            {
                if (command[j] == '\\' && command[j+1] != '\0')
                {
                    j++;
                    len++;
                } 
                len++; 
            }
            //allocate token string
            char *temp = (char *)malloc((len+3)*sizeof(char));
            int temp_idx = 1;

            temp[0] = command[i];

            for (int j = i+1; (command[j] != '\0'); j++)
            {
                //checks for escape characters like \n \0 \\ \" \xZw
                if (command[j] == '\\' && command[j+1] != '\0') 
                {
                    temp[temp_idx] = command[j];
                    temp_idx++;
                    temp[temp_idx] = command[j+1];
                    temp_idx++;
                    j++;
                    continue;
                }
                else{
                    temp[temp_idx] = command[j];
                    temp_idx++;
                }
                
                if (command[j] == '\"') { break; }
            }

            i = i+len+1;
            if (command[i] == '\0') { i--;}
           
            temp[temp_idx] = '\0';
            append(&head, temp, NO_ERROR);
        }
        //parse symbols that start with a letter (a-z) (A-Z)
        else if ((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z'))
        { 
            char *temp = (char *)calloc(4097, sizeof(char));
            int temp_idx = 0;
            int err_flag = 0;
            
            for (int j = i;strchr("\n\r\t\"\'; ()", command[j]) == NULL && command[j] != '\0'; j++)
            {
                
                if(strchr("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789+-*/%=<>!:", command[j]) == NULL)
                {
                    //put an error here
                    // break;
                    err_flag = 1;
                }
                temp[temp_idx] = command[j];
                temp_idx++;

                i = j;
            }
            if (err_flag == 0)
            {
                append(&head, temp, NO_ERROR);
            }
            else{
                append(&head, temp, INVALID_TOKEN);
            }
        }
        //parse symbols with '+-*/%=<>!:' non-alphanumeric symbols
        else if (strchr("+-*/%=<>!:", c) != NULL)
        {
            char *temp = (char *)calloc(4097, sizeof(char));
            int temp_idx = 0;
            int err_flag = 0;
            
            for (int j = i; strchr("\n\r\t\"\'; ()", command[j]) == NULL && command[j] != '\0'; j++)
            {
                if(strchr("+-*/%=<>!:", command[j]) == NULL)
                {
                    err_flag = 1;
                }
                temp[temp_idx] = command[j];
                temp_idx++;

                i = j;
            }
            if (err_flag == 0)
            {
                append(&head, temp, NO_ERROR);
            }
            else{
                append(&head, temp, INVALID_TOKEN);
            }
        }
        //tokenize special cases _ and &
        else if (c == '\'')
        {
            append(&head, createToken(c), NO_ERROR);
        }
        else if ((c == '_' || c == '&') && strchr("\n\r\t\"\'; ()", next_c) == NULL && next_c != '\0')
        {
            char *temp = (char *)calloc(4097, sizeof(char));
            int temp_idx = 0;

            for (int j = i; strchr("\n\r\t\"\'; ()", command[j]) == NULL && command[j] != '\0'; j++)
            {
                temp[temp_idx] = command[j];
                temp_idx++;

                i = j;
            }
            append(&head, temp, INVALID_TOKEN);
        }
        //tokenize special cases _ and &
        else if (c == '_' || c == '&')
        {
            append(&head, createToken(c), NO_ERROR);
        }
        else if (isspace((unsigned char)c)) 
        {
            continue;
        }
        else
        {
            char *temp = (char *)calloc(4097, sizeof(char));
            int temp_idx = 0;

            for (int j = i; strchr("\n\r\t\"\'; ()", command[j]) == NULL && command[j] != '\0'; j++)
            {
                temp[temp_idx] = command[j];
                temp_idx++;

                i = j;
            }
            append(&head, temp, INVALID_TOKEN);
        }
    }

    
    return head;
}