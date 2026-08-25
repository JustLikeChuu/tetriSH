#ifndef QUEUE_H
#define QUEUE_H
#include <stdbool.h>
#include <stdio.h>

// Suppress GCC/Clang compiler from throwing warnings when certain functions aren't used
// Does nothing if for some reason we're not using a compiler that supports this
#ifdef __GNUC__
#define IGNORE_UNUSED __attribute__((unused))
#else
#define IGNORE_UNUSED
#endif

// Macro version of Reference: https://www.geeksforgeeks.org/c/queue-in-c/
// So we can get fake Generic typing :D
// TYPE: Data Type,
// NAME: Will be the prefix of the queue & all functions (eg: IntQueue),
// MAX_SIZE: Maximum size of the queue
#define DEFINE_QUEUE(TYPE, NAME, MAX_SIZE)                               \
                                                                         \
    typedef struct                                                       \
    {                                                                    \
        TYPE items[MAX_SIZE];                                            \
        int front;                                                       \
        int rear;                                                        \
    } NAME##Queue;                                                       \
                                                                         \
    /* Function to initialize the queue */                               \
    static IGNORE_UNUSED void NAME##_init(NAME##Queue* q)                \
    {                                                                    \
        q->front = -1;                                                   \
        q->rear = 0;                                                     \
    }                                                                    \
                                                                         \
    /* Function to check if the queue is empty*/                         \
    static IGNORE_UNUSED bool NAME##_empty(NAME##Queue* q)               \
    {                                                                    \
        return q->front == q->rear - 1;                                  \
    }                                                                    \
                                                                         \
    /* Function to check if the queue is full */                         \
    static IGNORE_UNUSED bool NAME##_full(NAME##Queue* q)                \
    {                                                                    \
        return q->rear == MAX_SIZE;                                      \
    }                                                                    \
                                                                         \
    /* Function to add an element to the queue */                        \
    static IGNORE_UNUSED bool NAME##_enqueue(NAME##Queue* q, TYPE value) \
    {                                                                    \
        if (NAME##_full(q))                                              \
            return false;                                                \
        q->items[q->rear++] = value;                                     \
        return true;                                                     \
    }                                                                    \
                                                                         \
    /* Function to remove an element from the queue */                   \
    static IGNORE_UNUSED bool NAME##_dequeue(NAME##Queue* q)             \
    {                                                                    \
        if (NAME##_empty(q))                                             \
            return false;                                                \
        q->front++;                                                      \
        if (q->front == q->rear - 1) {                                   \
            q->front = -1;                                               \
            q->rear = 0;                                                 \
        };                                                               \
        return true;                                                     \
    }                                                                    \
                                                                         \
    /* Function to get the element at the front of the queue */          \
    static IGNORE_UNUSED TYPE* NAME##_peek(NAME##Queue* q)               \
    {                                                                    \
        if (NAME##_empty(q))                                             \
            return NULL;                                                 \
        return &q->items[q->front + 1];                                  \
    }
#endif
