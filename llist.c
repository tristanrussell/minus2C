#include <stdlib.h>
#include "llist.h"

LLIST *new_llist(void *item)
{
    LLIST *ret = (LLIST*)malloc(sizeof(LLIST));
    ret->item = item;
    ret->next = NULL;
    return ret;
}

ILLIST *new_illist(int i)
{
    ILLIST *ret = (ILLIST*)malloc(sizeof(ILLIST));
    ret->i = i;
    ret->next = NULL;
    return ret;
}

LLIST *join_llist(LLIST *list, LLIST *next)
{
    LLIST *curr = list;
    while (curr->next != NULL) curr = curr->next;
    curr->next = next;
    return list;
}

ILLIST *join_illist(ILLIST *list, ILLIST *next)
{
    ILLIST *curr = list;
    while (curr->next != NULL) curr = curr->next;
    curr->next = next;
    return list;
}

LLIST *append_llist(LLIST *list, void *next)
{
    return join_llist(list, new_llist(next));
}

int count_list(LLIST *list)
{
    LLIST *curr = list;
    int count = 0;
    while (curr != NULL) {
        count++;
        curr = curr->next;
    }
    return count;
}

int count_ilist(ILLIST *list)
{
    ILLIST *curr = list;
    int count = 0;
    while (curr != NULL) {
        count++;
        curr = curr->next;
    }
    return count;
}

int find_list(LLIST *list, void *item)
{
    LLIST *curr = list;
    while (curr != NULL) {
        if (curr->item == item) return 1;
        curr = curr->next;
    }
    return 0;
}