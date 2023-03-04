#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <semaphore.h>

#ifndef _LIST_
#define _LIST

typedef pthread_t tid_t;
typedef void (so_handler)(unsigned int);

typedef struct thread_t thread_t;


typedef struct ll_node_t {
thread_t *data;
struct ll_node_t *next;

} ll_node_t;



typedef struct linked_list_t {
    ll_node_t *head;
    unsigned int size;

} linked_list_t;



linked_list_t *ll_create(void);

ll_node_t *create_node(linked_list_t *list, thread_t *new_data);

void ll_add_nth_node(linked_list_t *list, unsigned int n, thread_t *new_data);

ll_node_t *ll_remove_nth_node(linked_list_t *list, unsigned int n);

unsigned int ll_get_size(linked_list_t *list);

void ll_free(linked_list_t **pp_list);

void enqueue(linked_list_t *list, thread_t *newNode);

ll_node_t *dequeue(linked_list_t *list);
#endif
