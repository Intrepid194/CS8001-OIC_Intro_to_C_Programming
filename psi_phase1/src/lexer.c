#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include "lexer.h"


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

Node *lex(Node *head, char* command) {
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
    return head;
}