#include "libeventbus.h"
#include <stdlib.h>

#pragma region Internal Types
typedef struct ListenerNode {
    EventListener fn;
    struct ListenerNode* next;
} ListenerNode;

// One slot per event type
// Holds the head of its listener list
typedef struct {
    ListenerNode* head;
} EventSlot;

// The event bus itself!
// Single global instance
static struct {
    EventSlot* slots;
    int num_events;
} event_bus;
#pragma endregion Internal Types

// Allocate one slot per event type
void event_bus_init(int num_events)
{
    event_bus.num_events = num_events;
    event_bus.slots = calloc(num_events, sizeof(EventSlot)); // zero memory so all heads start NULL
}

// Walk every slot and free its listener list, then free the slots array
void event_bus_free()
{
    for (int i = 0; i < event_bus.num_events; i++) {
        ListenerNode* node = event_bus.slots[i].head;
        while (node) {
            ListenerNode* next = node->next;
            free(node);
            node = next;
        }
    }
    free(event_bus.slots);
    event_bus.slots = NULL;
    event_bus.num_events = 0;
}

// Prepend a new listener node to the front of the slot's list
void event_bus_listen(int event_type, EventListener listener)
{
    ListenerNode* node = malloc(sizeof(ListenerNode));
    node->fn = listener;
    node->next = event_bus.slots[event_type].head;
    event_bus.slots[event_type].head = node;
}

// Walk the list with a pointer-to-pointer so we can unlink without a separate "previous" variable
// Function pointers compare equal iff they point to the
// same function, so this correctly identifies the right node to remove
void event_bus_stop_listening(int event_type, EventListener listener)
{
    ListenerNode** curr = &event_bus.slots[event_type].head;
    while (*curr) {
        if ((*curr)->fn == listener) {
            ListenerNode* to_free = *curr;
            *curr = (*curr)->next;
            free(to_free);
            return;
        }
        curr = &(*curr)->next;
    }
}

// Call every listener registered for this event type in order
void event_bus_trigger(int event_type, void* args)
{
    ListenerNode* node = event_bus.slots[event_type].head;
    while (node) {
        node->fn(args);
        node = node->next;
    }
}
