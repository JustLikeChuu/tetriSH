#ifndef LIBBR_CLIENTLL_H
#define LIBBR_CLIENTLL_H

#include "common.h"

// structs
struct Node {
    struct Node* next;
    Endpoint client;
};
typedef struct Node ClientNode;

typedef struct {
    ClientNode* head;
    uint32_t count;
} ClientLinkedList;

// list functions
int initalise_list(ClientLinkedList** list);
int add_to_list(ClientLinkedList* list, Endpoint* client);
int remove_from_list(ClientLinkedList* list, uint32_t id);
int get_from_list(ClientLinkedList* list, Endpoint* client, uint32_t id);
int get_id_array(ClientLinkedList* list, uint32_t** ids);
int free_list(ClientLinkedList** list_to_free);

#endif
