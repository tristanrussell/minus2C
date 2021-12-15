#include <stdlib.h>
#include "llist.h"

/**
 * Creates a new linked list and adds the first value to it.
 *
 * @param item : The first value to add.
 * @return : The new linked list.
 */
LLIST *new_llist(void *item)
{
    LLIST *ret = (LLIST*)malloc(sizeof(LLIST));
    ret->item = item;
    ret->next = NULL;
    return ret;
}

/**
 * Creates a new linked list of integers and adds the first value to it.
 *
 * @param i : The first integer to add to the list.
 * @return : The new linked list.
 */
ILLIST *new_illist(int i)
{
    ILLIST *ret = (ILLIST*)malloc(sizeof(ILLIST));
    ret->i = i;
    ret->next = NULL;
    return ret;
}

/**
 * Joins two linked list together.
 *
 * @param list : The first list to join, this will prepend the other list.
 * @param next : The second list to join, this will append the other list.
 * @return : The new joined linked list.
 */
LLIST *join_llist(LLIST *list, LLIST *next)
{
    LLIST *curr = list;
    while (curr->next != NULL) curr = curr->next;
    curr->next = next;
    return list;
}

/**
 * Joins two integer linked list together.
 *
 * @param list : The first list to join, this will prepend the other list.
 * @param next : The second list to join, this will append the other list.
 * @return : The new joined integer linked list.
 */
ILLIST *join_illist(ILLIST *list, ILLIST *next)
{
    ILLIST *curr = list;
    while (curr->next != NULL) curr = curr->next;
    curr->next = next;
    return list;
}

/**
 * Appends an item to a linked list.
 *
 * @param list : The list to be appended to.
 * @param next : The item to append to the linked list.
 * @return : The resulting linked list.
 */
LLIST *append_llist(LLIST *list, void *next)
{
    return join_llist(list, new_llist(next));
}

/**
 * Counts the number of items in a linked list.
 *
 * @param list : The linked list to count.
 * @return : The number of items in the linked list.
 */
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

/**
 * Counts the number of items in an integer linked list.
 *
 * @param list : The integer linked list to count.
 * @return : The number of items in the integer linked list.
 */
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

/**
 * Checks if an item is in a linked list.
 *
 * @param list : The linked list to search.
 * @param item : The item to search for.
 * @return : 1 if the item is found, 0 otherwise.
 */
int find_list(LLIST *list, void *item)
{
    LLIST *curr = list;
    while (curr != NULL) {
        if (curr->item == item) return 1;
        curr = curr->next;
    }
    return 0;
}