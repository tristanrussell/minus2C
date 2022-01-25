#include <stdlib.h>
#include <stdio.h>
#include "value.h"

/**
 * Creates a new value and marks it as uninitialised.
 *
 * @param type : The type of the variable.
 * @return : The new uninitialised variable.
 */
VALUE* new_null(int type)
{
    VALUE* ans = (VALUE*)malloc(sizeof(VALUE));
    if (ans==NULL) {
        printf("Error! memory not allocated.\n");
        exit(EXIT_FAILURE);
    }
    ans->type = mmcNULL;
    ans->v.integer = type;
    return ans;
}

/**
 * Creates a new value of type return.
 *
 * @param val : The value being returned.
 * @return : The return value enclosing the returned value.
 */
VALUE* new_return(VALUE *val)
{
    VALUE* ans = (VALUE*)malloc(sizeof(VALUE));
    if (ans==NULL) {
        printf("Error! memory not allocated.\n");
        exit(EXIT_FAILURE);
    }
    ans->type = mmcRETURN;
    ans->v.ret = val;
    return ans;
}

/**
 * Creates a new integer value structure.
 *
 * @param v : The integer being enclosed in the value structure.
 * @return : The new value structure.
 */
VALUE* new_int(int v)
{
    VALUE* ans = (VALUE*)malloc(sizeof(VALUE));
    if (ans==NULL) {
        printf("Error! memory not allocated.\n");
        exit(EXIT_FAILURE);
    }
    ans->type = mmcINT;
    ans->v.integer = v;
    return ans;
}

/**
 * Creates a new closure value structure and creates the proc structure that
 * the value encloses.
 *
 * @param env : The frame of the function.
 * @param code : The body of code for the function.
 * @return : The new closure value structure.
 */
VALUE *new_closure(FRAME *env, NODE *code)
{
    VALUE *ans = (VALUE*)malloc(sizeof(VALUE));
    CLOSURE *func = (CLOSURE*)malloc(sizeof(CLOSURE));
    if (ans==NULL || func==NULL) {
        printf("Error! memory not allocated.\n");
        exit(EXIT_FAILURE);
    }

    func->env = env;
    func->ids = NULL;
    func->type = mmcINT;
    func->code = code;

    ans->type = mmcCLOSURE;
    ans->v.closure = func;

    return ans;
}

/**
 * Prints a value to the command line.
 *
 * @param v : The value structure to be printed.
 */
void mmc_print(VALUE* v)
{
    if (v==NULL) return;
    switch (v->type) {
        case mmcINT:
            printf("%d\n",v->v.integer);
            break;
        case mmcBOOL:
        case mmcSTRING:
        default:
            printf("unknown type code %d (%p) in mmc_print\n",v->type,v);
    }
}