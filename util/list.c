#include "util/list.h"

void list_link_init(list_link_t *link, void *parent) {
    link->next = NULL;
    link->prev = NULL;
    link->parent = parent;
}

void list_init(list_t *l) {
    l->head = NULL;
    l->tail = NULL;
    l->size = 0;
}

void list_insert_front(list_t *l, list_link_t *link) {
    if (l->size == 0) {
        l->tail = link;
    } else {
        l->head->prev = link;
    }
    link->next = l->head;
    l->head = link;
    l->size++;
}

void list_insert_back(list_t *l, list_link_t *link) {
    if (l->size == 0) {
        l->head = link;
    } else {
        l->tail->next = link;
    }
    link->prev = l->tail;
    l->tail = link;
    l->size++;
}

void list_insert(list_t *l, list_link_t *link) {
    list_insert_back(l, link);
}

list_link_t *list_remove_front(list_t *l) {
    if (l->size == 0) {
        return NULL;
    } else if (l->head == l->tail) {
        l->tail = NULL;
    }

    list_link_t *ret = l->head;
    l->head = l->head->next;
    l->size--;
    return ret;
}

list_link_t *list_remove_back(list_t *l) {
    if (l->size == 0) {
        return NULL;
    } else if (l->head == l->tail) {
        l->head = NULL;
    }

    list_link_t *ret = l->tail;
    l->tail = l->tail->prev;
    l->size--;
    return ret;
}

void list_remove_link(list_t *l, list_link_t *link) {
    for (list_link_t *element = l->head; element != NULL;
                                         element = element->next) {
        if (element == link) {
            if (element->prev) {
                element->prev->next = element->next;
            }
            if (element->next) {
                element->next->prev = element->prev;
            }
            l->size--;
            return;
        }
    }
}

