#ifndef MINUS2C_LLIST_H
#define MINUS2C_LLIST_H

typedef struct llist {
    void *item;
    struct llist *next;
} LLIST;

typedef struct illist {
    int i;
    struct illist *next;
} ILLIST;

LLIST *new_llist(void *item);

ILLIST *new_illist(int i);

LLIST *join_llist(LLIST *list, LLIST *next);

ILLIST *join_illist(ILLIST *list, ILLIST *next);

LLIST *append_llist(LLIST *list, void *next);

int count_list(LLIST *list);

int count_ilist(ILLIST *list);

int find_list(LLIST *list, void *item);

#endif //MINUS2C_LLIST_H
