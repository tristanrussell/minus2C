#include <stdlib.h>
#include <stdio.h>
#include "env.h"
#include "value.h"
#include "interpreter.h"
#include "C.tab.h"

FRAME *new_frame()
{
    FRAME *f = (FRAME*)malloc(sizeof(FRAME));
    f->bindings = NULL;
    f->next = NULL;
    return f;
}

BINDING *make_binding(NODE *node, VALUE *value, BINDING *bindings)
{
    TOKEN *name = (TOKEN*)node;

    BINDING *newBind = malloc(sizeof(BINDING));
    if (newBind != NULL) {
        newBind->name = name;
        newBind->val = value;
        newBind->next = bindings;
        return newBind;
    }
    exit(EXIT_FAILURE);
}

VALUE *init_vars(NODE *ids, NODE *args, FRAME *newEnv, FRAME *oldEnv)
{
    if (ids == NULL || args == NULL) {
        printf("ids or args missing.\n");
        return (VALUE*)0;
    }

    TOKEN *tok;
    VALUE *val = (VALUE*)0;
    int type;

    switch (ids->type) {
        case ',':
            init_vars(ids->left, args->left, newEnv, oldEnv);
            init_vars(ids->right, args->right, newEnv, oldEnv);
            break;
        case '~':
            tok = (TOKEN*)interpret(ids->right, newEnv);
            type = interpret(ids->left, newEnv)->type;
            switch (type) {
                case INT:
                    type = mmcINT;
                    break;
                case FUNCTION:
                    type = mmcCLOSURE;
                    break;
            }
            declare_name(tok, newEnv, type);
            val = interpret(args, oldEnv);
            if (val->type == IDENTIFIER) val = lookup_name((TOKEN*)val, oldEnv);
            assign_name(tok, newEnv, val);
            break;
    }

    return val;
}

FRAME *extend_frame(FRAME *env, NODE *ids, NODE *args)
{
    FRAME * newenv = new_frame();
    if (ids != NULL && ids->type != VOID) init_vars(ids, args, newenv, env);
    return newenv;
}

VALUE *lookup_name(TOKEN *name, FRAME *frame)
{
    while (frame != NULL) {
        BINDING *bindings = frame->bindings;
        while (bindings != NULL) {
            if (bindings->name == name) {
                if (bindings->val->type == mmcNULL) {
                    printf("Attempting to use uninitialised variable.\n");
                    exit(EXIT_FAILURE);
                }
                return bindings->val;
            }
            bindings = bindings->next;
        }
        frame = frame->next;
    }
    printf("Name not found.");
    exit(EXIT_FAILURE);
}

VALUE *assign_name(TOKEN *name, FRAME *frame, VALUE *value)
{
    while (frame != NULL) {
        BINDING *bindings = frame->bindings;
        while (bindings != NULL) {
            if (bindings->name == name) {
                if (bindings->val->type == mmcNULL) {
                    if (bindings->val->v.integer == value->type) {
                        bindings->val = value;
                        return value;
                    }
                } else if (bindings->val->type == value->type) {
                    bindings->val = value;
                    return value;
                }
                printf("Assigning wrong type to variable. Expected:%d Received:%d\n", bindings->val->v.integer, value->type);
                exit(EXIT_FAILURE);
            }
            bindings = bindings->next;
        }
        frame = frame->next;
    }
    exit(EXIT_FAILURE);
}

VALUE *declare_name(TOKEN *name, FRAME *frame, int type)
{
    BINDING *bindings = frame->bindings;
    while (bindings != NULL) {
        if (bindings->name == name) {
            printf("Error, duplicate variable declaration.");
            exit(EXIT_FAILURE);
        }
        bindings = bindings->next;
    }
    bindings = frame->bindings;
    BINDING *newBind = malloc(sizeof(BINDING));
    if (newBind != NULL) {
        newBind->name = name;
        newBind->val = new_null(type);
        newBind->next = bindings;
        frame->bindings = newBind;
        return newBind->val;
    }
    exit(EXIT_FAILURE);
}

VALUE *declare_closure(TOKEN *name, FRAME *frame, NODE *code, NODE *ids, int type)
{
    BINDING *bindings = frame->bindings;
    while (bindings != NULL) {
        if (bindings->name == name) {
            printf("Error, duplicate function declaration.");
            exit(EXIT_FAILURE);
        }
        bindings = bindings->next;
    }
    bindings = frame->bindings;
    BINDING *newBind = malloc(sizeof(BINDING));
    printf("Declaration: %d\n", name);
    if (newBind != NULL) {
        newBind->name = name;
        newBind->val = new_closure(frame, code);
        newBind->val->v.closure->type = type;
        newBind->val->v.closure->ids = ids;
        newBind->next = bindings;
        frame->bindings = newBind;
        return newBind->val;
    }
    exit(EXIT_FAILURE);
}

void print_names(FRAME *frame)
{
    while (frame != NULL) {
        BINDING *bindings = frame->bindings;
        while (bindings != NULL) {
            printf("%s\n", bindings->name->lexeme);
            bindings = bindings->next;
        }
        printf("\n\n");
        frame = frame->next;
    }
}