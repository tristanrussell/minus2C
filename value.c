#include <stdlib.h>
#include <stdio.h>
#include "value.h"

//VALUE *empty_value(int type) {
//    VALUE *v = (VALUE*)malloc(sizeof(VALUE));
//    v->type = type;
//    return v;
//}
//
//VALUE *new_value(TOKEN *t) {
//    VALUE *v = (VALUE*)malloc(sizeof(VALUE));
//
//    switch (t->type) {
//        case CONSTANT:
//            v->type = INT;
//            v->v.integer = t->value;
//            break;
//    }
//
//    return v;
//}

VALUE* new_null(int type)
{
    VALUE* ans = (VALUE*)malloc(sizeof(VALUE));
    if (ans==NULL) {
        printf("Error! memory not allocated.");
        exit(EXIT_FAILURE);
    }
    ans->type = mmcNULL;
    ans->v.integer = type;
    return ans;
}

VALUE* new_return(VALUE *val)
{
    VALUE* ans = (VALUE*)malloc(sizeof(VALUE));
    if (ans==NULL) {
        printf("Error! memory not allocated.");
        exit(EXIT_FAILURE);
    }
    ans->type = mmcRETURN;
    ans->v.ret = val;
    return ans;
}

VALUE* new_int(int v)
{
    VALUE* ans = (VALUE*)malloc(sizeof(VALUE));
    if (ans==NULL) {
        printf("Error! memory not allocated.");
        exit(EXIT_FAILURE);
    }
    ans->type = mmcINT;
    ans->v.integer = v;
    return ans;
}

VALUE *new_closure(FRAME *env, NODE *code)
{
    VALUE *ans = (VALUE*)malloc(sizeof(VALUE));
    CLOSURE *func = (CLOSURE*)malloc(sizeof(CLOSURE));
    if (ans==NULL || func==NULL) {
        printf("Error! memory not allocated.");
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