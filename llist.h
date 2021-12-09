#ifndef MINUS2C_LLIST_H
#define MINUS2C_LLIST_H

typedef struct llist {
    void *item;
    struct llist *next;
} LLIST;

LLIST *new_llist(void *item);

LLIST *join_llist(LLIST *list, LLIST *next);

LLIST *append_llist(LLIST *list, void *next);

int count_list(LLIST *list);

int find_list(LLIST *list, void *item);

#endif //MINUS2C_LLIST_H
