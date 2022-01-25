#include <stdlib.h>
#include <stdio.h>
#include "env.h"
#include "value.h"
#include "interpreter.h"
#include "C.tab.h"

/**
 * Creates a new frame, allocates the memory and sets the variables to NULL.
 *
 * @return : The new frame.
 */
FRAME *new_frame()
{
    FRAME *f = (FRAME*)malloc(sizeof(FRAME));
    f->bindings = NULL;
    f->next = NULL;
    return f;
}

/**
 * Takes the root node of a list of parameters and the root node of a list of
 * arguments and initialises all of the parameters in the new frame with the
 * values of the arguments.
 *
 * @param ids : The parameters being initialised.
 * @param args : The arguments being used to initialise the parameters.
 * @param newEnv : The new frame for the parameters to be initialised in.
 * @param oldEnv : The environment containing the arguments being used.
 * @return : If one parameter is being initialised then it's value is returned
 *           otherwise NULL is returned.
 */
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

/**
 * Creates a new frame as a child of another frame. Also initialises all of the
 * parameters of the new frame.
 *
 * @param env : The parent frame.
 * @param ids : The parameters to be initialised.
 * @param args : The arguments to initialise the parameters.
 * @return : The new frame.
 */
FRAME *extend_frame(FRAME *env, NODE *ids, NODE *args)
{
    FRAME * newenv = new_frame();
    if (ids != NULL && ids->type != VOID) init_vars(ids, args, newenv, env);
    return newenv;
}

/**
 * Looks up a variable in an environment.
 *
 * @param name : The variable being looked up.
 * @param frame : The current frame to start the search in.
 * @return : The value of the variable if found.
 */
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
    return NULL;
}

/**
 * Assigns a new value to a variable.
 *
 * @param name : The variable being modified.
 * @param frame : The current frame to start the search for the variable in.
 * @param value : The new value to assign to the variable.
 * @return : The new value of the variable.
 */
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
                printf("Assigning wrong type to variable.\n");
                exit(EXIT_FAILURE);
            }
            bindings = bindings->next;
        }
        frame = frame->next;
    }
    printf("Assigning value to non-existent variable.\n");
    exit(EXIT_FAILURE);
}

/**
 * Creates a new binding for a new variable in the current frame.
 *
 * @param name : The new variable.
 * @param frame : The current frame.
 * @param type : The type of the new variable.
 * @return : The new NULL value of the new variable.
 */
VALUE *declare_name(TOKEN *name, FRAME *frame, int type)
{
    BINDING *bindings = frame->bindings;
    while (bindings != NULL) {
        if (bindings->name == name) {
            printf("Error, duplicate variable declaration.\n");
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
    printf("Error declaring variable.\n");
    exit(EXIT_FAILURE);
}

/**
 * Creates a new binding for a new closure in the current frame.
 *
 * @param name : The token name of the new closure.
 * @param frame : The frame to declare the closure in.
 * @param code : The body of code for the closure.
 * @param ids : The parameters for the closure.
 * @param type : The return type of the new closure.
 * @return : The closure as a value structure.
 */
VALUE *declare_closure(TOKEN *name, FRAME *frame, NODE *code, NODE *ids, int type)
{
    BINDING *bindings = frame->bindings;
    while (bindings != NULL) {
        if (bindings->name == name) {
            printf("Error, duplicate function declaration.\n");
            exit(EXIT_FAILURE);
        }
        bindings = bindings->next;
    }
    bindings = frame->bindings;
    BINDING *newBind = malloc(sizeof(BINDING));
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