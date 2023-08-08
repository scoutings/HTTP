#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <stdbool.h>

#include "queue.h"
#include "talk.h"

struct Queue {
    int head;
    int tail;
    int size;
    int capacity;
    int *items;
};

Queue *queue_create(int capacity) {
    Queue *q = (Queue *) malloc(sizeof(Queue));
    if (q) {
        q->head = 0;
        q->tail = 0;
        q->size = 0;
        q->capacity = capacity;
        q->items = (int *) calloc(capacity, sizeof(int));
        if (!q->items) {
            free(q);
            q = NULL;
        }
    }
    return q;
}

void queue_delete(Queue **q) {
    if (*q && (*q)->items) {
        free((*q)->items);
        free(*q);
        *q = NULL;
    }
    return;
}

bool queue_empty(Queue *q) {
    return !q->size;
}

bool queue_full(Queue *q) {
    return q->size == q->capacity;
}

int queue_size(Queue *q) {
    return q->size;
}

bool enqueue(Queue *q, int x) {
    if (!queue_full(q)) {
        q->items[q->head] = x;
        // if the head is at the end we reset head to 0
        if (q->head == q->capacity - 1) {
            q->head = 0;
        } else {
            q->head++;
        }
        q->size++;
        return true;
    } else {
        return false;
    }
}

bool dequeue(Queue *q, int *x) {
    if (!queue_empty(q)) {
        *x = q->items[q->tail];
        // if the tail has reached the end we reset tail to 0
        if (q->tail == q->capacity - 1) {
            q->tail = 0;
        } else {
            q->tail++;
        }
        q->size--;
        return true;
    } else {
        return false;
    }
}

void queue_print(Queue *q) {
    for (int i = 0; i < q->capacity; i++) {
        printf("Element: %d\n", q->items[i]);
    }
    return;
}

// struct Queue_one {
//     int size;
//     Read_request *val;
// };

// Queue_one *create_queue() {
//     Queue_one *ret_val = (Queue_one *) malloc(sizeof(Queue_one));
//     if (ret_val) {
//         ret_val->size = 0;
//         ret_val->val = NULL;
//     }
//     return ret_val;
// }

// void destroy_queue(Queue_one **q) {
//     if (*q) {
//         if ((*q)->val) {
//             free((*q)->val);
//         }
//         free(*q);
//         *q = NULL;
//     }
//     return;
// }

// int enqueue(Queue_one *q, Read_request *v) {
//     int ret_val = -1;
//     if (q && q->size == 0) {
//         q->val = v;
//         q->size = 1;
//         ret_val = 0;
//     } else {
//         printf("!!! ERROR !!!\n");
//     }
//     return ret_val;
// }

// Read_request *dequeue(Queue_one *q) {
//     Read_request *ret_val = NULL;
//     if (q && q->size == 1) {
//         ret_val = q->val;
//         q->val = NULL;
//         q->size = 0;
//     } else {
//         printf("!!! ERROR !!!\n");
//     }
//     return ret_val;
// }

// int get_length(Queue_one *q) {
//     int ret_val = -1;
//     if (q) {
//         ret_val = q->size;
//     } else {
//         printf("!!! ERROR !!!\n");
//     }
//     return ret_val;
// }

// ---------------------------------------------------

// typedef struct Node Node;
// struct Node {
//     Read_request *val;
//     Node *next;
// };

// struct Queue {
//     int max;
//     int length;
//     Node *head;
// };

// Queue *create_queue(int max) {
//     Queue *ret_val = (Queue *) malloc(sizeof(Queue));
//     if (ret_val) {
//         ret_val->length = 0;
//         ret_val->max = max;
//         ret_val->head = NULL;
//     } else {
//         printf("!!! Error !!!\n");
//     }
//     return ret_val;
// }

// void destroy_queue(Queue **q) {
//     if (*q) {
//         while ((*q)->length != 0) {
//             dequeue(*q);
//         }
//         free(*q);
//         *q = NULL;
//     }
//     return;
// }

// int get_length(Queue *q) {
//     int ret_val = -1;
//     if (q) {
//         ret_val = q->length;
//     } else {
//         printf("!!! Error !!!\n");
//     }
//     return ret_val;
// }

// int get_max(Queue *q) {
//     int ret_val = -1;
//     if (q) {
//         ret_val = q->max;
//     } else {
//         printf("!!! ERROR !!!\n");
//     }
//     return ret_val;
// }

// int enqueue(Queue *q, Read_request *i) {
//     int ret_val = -1;
//     Node *n_add = (Node *) malloc(sizeof(Node));
//     // If the q exists and the node was successfully created
//     if (q && n_add && q->length < q->max) {
//         // Set the values of the Node
//         n_add->val = i;
//         n_add->next = NULL;
//         // Go to the end of the linked list and add
//         Node *trav = q->head;
//         if (trav) {
//             while (trav->next != NULL) {
//                 trav = trav->next;
//             }
//             trav->next = n_add;
//             // Else if this is the first to add then make it the head
//         } else {
//             q->head = n_add;
//         }
//         q->length += 1;
//         ret_val = 0;
//     } else {
//         printf("!!! Error !!!\n");
//     }
//     return ret_val;
// }

// Read_request *dequeue(Queue *q) {
//     Read_request *ret_val = NULL;
//     if (q) {
//         if (q->length != 0) {
//             Node *deq_node = q->head;
//             ret_val = deq_node->val;
//             q->head = deq_node->next;
//             free(deq_node);
//             deq_node = NULL;
//             q->length -= 1;
//         }
//     } else {
//         printf("!!! Error !!!\n");
//     }
//     return ret_val;
// }

// void print_queue(Queue *q) {
//     if (q) {
//         printf("Length: [%d]\n", q->length);
//         Node *trav = q->head;
//         if (trav) {
//             while (trav->next != NULL) {
//                 printf("%d<-", trav->val->connfd);
//                 trav = trav->next;
//             }
//             printf("%d\n", trav->val->connfd);
//         }
//     }
//     return;
// }
