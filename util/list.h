#ifndef __LIST_H__
#define __LIST_H__

#include "util/ktypes.h"

// list stuff
typedef struct list_link {
    struct list_link *next;
    struct list_link *prev;
    void *parent;
} list_link_t;

void list_link_init(list_link_t *link, void *parent);

typedef struct list {
    list_link_t *head;
    list_link_t *tail;
    long size;
} list_t;

void list_init(list_t *l);

void list_insert_front(list_t *l, list_link_t *link);

void list_insert_back(list_t *l, list_link_t *link);

void list_insert(list_t *l, list_link_t *link);

list_link_t *list_remove_front(list_t *l);

list_link_t *list_remove_back(list_t *l);

void list_remove_link(list_t *l, list_link_t *link);

#endif // __LIST_H__
