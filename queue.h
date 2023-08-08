#ifndef __QUEUE_H__
#define __QUEUE_H__

#include <stdbool.h>

#include "talk.h"

typedef struct Queue Queue;

Queue *queue_create(int capacity);

void queue_delete(Queue **q);

bool queue_empty(Queue *q);

bool queue_full(Queue *q);

int queue_size(Queue *q);

bool enqueue(Queue *q, int x);

bool dequeue(Queue *q, int *x);

void queue_print(Queue *q);

// typedef struct Queue_one Queue_one;

// Queue_one *create_queue();

// void destroy_queue(Queue_one **q);

// int enqueue(Queue_one *q, Read_request *v);

// Read_request *dequeue(Queue_one *q);

// int get_length(Queue_one *q);

// -------------------------------------------

// typedef struct Queue Queue;

// Queue *create_queue(int max);

// void destroy_queue(Queue **q);

// int get_length(Queue *q);

// int get_max(Queue *q);

// int enqueue(Queue *q, Read_request *i);

// Read_request *dequeue(Queue *q);

// void print_queue(Queue *q);

#endif
