#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "interpreter.h"
#include "value.h"
#include "env.h"
#include "C.tab.h"

// Predefined methods
VALUE *print_int(NODE*, FRAME*);
VALUE *read_int();

VALUE *interpret(NODE*, FRAME*);

/**
 * Given a node representing a list of variable declaration, this function will
 * add each of the variables to the current frame.
 *
 * @param tree : The node of the AST containing the list of variable
 *               declarations.
 * @param frame : The current frame for the code being interpreted.
 * @param type : The type of the variables being declared.
 * @return : If only a single variable is declared then the value of the
 *           variable will be returned otherwise NULL is returned.
 */
VALUE *create_vars(NODE *tree, FRAME *frame, int type)
{
    TOKEN *t = (TOKEN*)tree;
    VALUE *val = (VALUE*)0;

    switch (type) {
        case INT:
            type = mmcINT;
            break;
        case FUNCTION:
            type = mmcCLOSURE;
            break;
    }

    switch (tree->type) {
        case ',':
            create_vars(tree->left, frame, type);
            create_vars(tree->right, frame, type);
            break;
        case '=':
            val = create_vars(tree->left, frame, type);
            interpret(tree, frame);
            break;
        case LEAF:
            val = create_vars(tree->left, frame, type);
            break;
        case IDENTIFIER:
            val = declare_name(t, frame, type);
    }

    return val;
}

/**
 * The method for interpreting procedure declaration. Adds the procedure to the
 * frame and if it is the main function then calls it immediately.
 *
 * @param tree : The node of the AST declaring a new function.
 * @param frame : The current frame for the code being interpreted.
 * @return : The return value from declaring the function.
 */
VALUE *create_closure(NODE *tree, FRAME *frame)
{
    if (tree->type != 'D') return NULL;

    NODE *dec = tree->left;
    NODE *code = tree->right;

    if (dec->type != 'd' || code == NULL) {
        printf("Error in function declaration.\n");
        exit(EXIT_FAILURE);
    }

    VALUE *type = interpret(dec->left, frame);

    NODE *func = dec->right;

    if (func->type != 'F') {
        printf("Error in function declaration.\n");
        exit(EXIT_FAILURE);
    }

    TOKEN *name = (TOKEN*)interpret(func->left, frame);
    NODE *ids = func->right;

    if (name == NULL || ids == NULL) {
        printf("Error in function declaration.\n");
        exit(EXIT_FAILURE);
    }

    if (strcmp(name->lexeme, "main") == 0) { // If this is main then run the function
        FRAME *newFrame = extend_frame(frame, NULL, NULL);
        newFrame->next = frame;
        return interpret(code, newFrame);
    } else return declare_closure(name, frame, code, ids, type->type);
}

/**
 * The method for interpreting function calls. Checks if it's a built in
 * function and calls the appropriate function if required. Otherwise searches
 * for the declared function.
 *
 * @param name : The token name of the function being called.
 * @param args : The arguments as required for the function.
 * @param env : The current frame for the code being interpreted.
 * @return : The return value from the function.
 */
VALUE *lexical_call_method(TOKEN *name, NODE *args, FRAME *env)
{
    VALUE *v = lookup_name(name, env);
    if (v == NULL || v->type != mmcCLOSURE) {
        if (strcmp(name->lexeme, "print_int") == 0) return print_int(args, env);
        else if (strcmp(name->lexeme, "read_int") == 0) return read_int();
        printf("Non-existent function called.\n");
        exit(EXIT_FAILURE);
    }
    CLOSURE *f = v->v.closure;
    FRAME *newenv = extend_frame(env, f->ids, args);
    newenv->next = f->env;
    return interpret(f->code, newenv);
}

/**
 * The function for interpreting the built in print_int function.
 *
 * @param args : The argument required by the function, an integer node.
 * @param frame : The current frame for the code being interpreted.
 * @return : A NULL pointer.
 */
VALUE *print_int(NODE *args, FRAME *frame)
{
    VALUE *arg = interpret(args, frame);
    if (arg->type == IDENTIFIER) {
        arg = lookup_name((TOKEN*)arg, frame);
    }

    printf("%d\n", arg->v.integer);

    return NULL;
}

/**
 * The function for interpreting the built in read_int function.
 *
 * @return : A VALUE representing the integer entered on the command line.
 */
VALUE *read_int()
{
    int toReturn;
    int result = scanf("%d", &toReturn);
    if (result == EOF) {
        printf("Error, cannot read input.\n");
        exit(EXIT_FAILURE);
    }
    if (result == 0) {
        printf("Error, invalid input.\n");
        exit(EXIT_FAILURE);
    }

    return new_return(new_int(toReturn));
}

/**
 * The main interpret function that is called recursively to traverse the AST.
 *
 * @param tree : The current node to interpret.
 * @param frame : The current frame for the code being interpreted.
 * @return : A pointer to the VALUE structure constructed from interpreting the
 *           current node.
 */
VALUE *interpret(NODE *tree, FRAME *frame)
{
    TOKEN *t = (TOKEN *)tree;
    VALUE *leftLeaf;
    VALUE *rightLeaf;

    switch (tree->type) {
        default: printf("???\n"); return NULL;
        case '%':
            leftLeaf = interpret(tree->left, frame);
            rightLeaf = interpret(tree->right, frame);
            if (leftLeaf->type == IDENTIFIER) leftLeaf = lookup_name((TOKEN*)leftLeaf, frame);
            if (rightLeaf->type == IDENTIFIER) rightLeaf = lookup_name((TOKEN*)rightLeaf, frame);
            if (leftLeaf->type == mmcINT && rightLeaf->type == mmcINT) {
                return new_int(leftLeaf->v.integer % rightLeaf->v.integer);
            }
            return NULL;
        case '*':
            leftLeaf = interpret(tree->left, frame);
            rightLeaf = interpret(tree->right, frame);
            if (leftLeaf->type == IDENTIFIER) leftLeaf = lookup_name((TOKEN*)leftLeaf, frame);
            if (rightLeaf->type == IDENTIFIER) rightLeaf = lookup_name((TOKEN*)rightLeaf, frame);
            if (leftLeaf->type == mmcINT && rightLeaf->type == mmcINT) {
                return new_int(leftLeaf->v.integer * rightLeaf->v.integer);
            }
            return NULL;
        case '+':
            leftLeaf = interpret(tree->left, frame);
            rightLeaf = interpret(tree->right, frame);
            if (leftLeaf->type == IDENTIFIER) leftLeaf = lookup_name((TOKEN*)leftLeaf, frame);
            if (rightLeaf->type == IDENTIFIER) rightLeaf = lookup_name((TOKEN*)rightLeaf, frame);
            if (leftLeaf->type == mmcINT && rightLeaf->type == mmcINT) {
                return new_int(leftLeaf->v.integer + rightLeaf->v.integer);
            }
            return NULL;
        case '-':
            leftLeaf = interpret(tree->left, frame);
            rightLeaf = interpret(tree->right, frame);
            if (leftLeaf->type == IDENTIFIER) leftLeaf = lookup_name((TOKEN*)leftLeaf, frame);
            if (rightLeaf->type == IDENTIFIER) rightLeaf = lookup_name((TOKEN*)rightLeaf, frame);
            if (leftLeaf->type == mmcINT && rightLeaf->type == mmcINT) {
                return new_int(leftLeaf->v.integer - rightLeaf->v.integer);
            }
            return NULL;
        case '/':
            leftLeaf = interpret(tree->left, frame);
            rightLeaf = interpret(tree->right, frame);
            if (leftLeaf->type == IDENTIFIER) leftLeaf = lookup_name((TOKEN*)leftLeaf, frame);
            if (rightLeaf->type == IDENTIFIER) rightLeaf = lookup_name((TOKEN*)rightLeaf, frame);
            if (leftLeaf->type == mmcINT && rightLeaf->type == mmcINT) {
                return new_int(leftLeaf->v.integer / rightLeaf->v.integer);
            }
            return NULL;
        case ';':
            leftLeaf = interpret(tree->left, frame);
            if (leftLeaf != NULL) {
                switch (leftLeaf->type) {
                    case CONTINUE:
                    case BREAK:
                    case mmcRETURN:
                        return leftLeaf;
                }
            }
            return interpret(tree->right, frame);
        case '<':
            leftLeaf = interpret(tree->left, frame);
            rightLeaf = interpret(tree->right, frame);
            if (leftLeaf->type == IDENTIFIER) leftLeaf = lookup_name((TOKEN*)leftLeaf, frame);
            if (rightLeaf->type == IDENTIFIER) rightLeaf = lookup_name((TOKEN*)rightLeaf, frame);
            if(leftLeaf->type == mmcINT && rightLeaf->type == mmcINT) {
                return new_int(leftLeaf->v.integer < rightLeaf->v.integer);
            }
            return NULL;
        case '=':
            leftLeaf = interpret(tree->left, frame);
            rightLeaf = interpret(tree->right, frame);
            if (rightLeaf->type == IDENTIFIER) rightLeaf = lookup_name((TOKEN*)rightLeaf, frame);
            assign_name((TOKEN*)leftLeaf, frame, rightLeaf);
            return NULL;
        case '>':
            leftLeaf = interpret(tree->left, frame);
            rightLeaf = interpret(tree->right, frame);
            if (leftLeaf->type == IDENTIFIER) leftLeaf = lookup_name((TOKEN*)leftLeaf, frame);
            if (rightLeaf->type == IDENTIFIER) rightLeaf = lookup_name((TOKEN*)rightLeaf, frame);
            if(leftLeaf->type == mmcINT && rightLeaf->type == mmcINT) {
                return new_int(leftLeaf->v.integer > rightLeaf->v.integer);
            }
            return NULL;
        case 'D':
            return create_closure(tree, frame);
        case 'F':
            return NULL;
        case 'd':
            return NULL;
        case '~':
            if (tree->left->type == '~' || tree->left->type == 'D') {
                leftLeaf = interpret(tree->left, frame);
                if (leftLeaf != NULL && leftLeaf->type == mmcRETURN) return leftLeaf;
                rightLeaf = interpret(tree->right, frame);
                if (rightLeaf != NULL && rightLeaf->type == mmcRETURN) return rightLeaf;
            } else create_vars(tree->right, frame, interpret(tree->left, frame)->type);
            return NULL;
        case IDENTIFIER:
            return (VALUE*)tree;
        case CONSTANT:
            return new_int(t->value);
        case STRING_LITERAL:
            return (VALUE*)tree;
        case LE_OP:
            leftLeaf = interpret(tree->left, frame);
            rightLeaf = interpret(tree->right, frame);
            if (leftLeaf->type == IDENTIFIER) leftLeaf = lookup_name((TOKEN*)leftLeaf, frame);
            if (rightLeaf->type == IDENTIFIER) rightLeaf = lookup_name((TOKEN*)rightLeaf, frame);
            if(leftLeaf->type == mmcINT && rightLeaf->type == mmcINT) {
                return new_int(leftLeaf->v.integer <= rightLeaf->v.integer);
            }
            return NULL;
        case GE_OP:
            leftLeaf = interpret(tree->left, frame);
            rightLeaf = interpret(tree->right, frame);
            if (leftLeaf->type == IDENTIFIER) leftLeaf = lookup_name((TOKEN*)leftLeaf, frame);
            if (rightLeaf->type == IDENTIFIER) rightLeaf = lookup_name((TOKEN*)rightLeaf, frame);
            if(leftLeaf->type == mmcINT && rightLeaf->type == mmcINT) {
                return new_int(leftLeaf->v.integer >= rightLeaf->v.integer);
            }
            return NULL;
        case EQ_OP:
            leftLeaf = interpret(tree->left, frame);
            rightLeaf = interpret(tree->right, frame);
            if (leftLeaf->type == IDENTIFIER) leftLeaf = lookup_name((TOKEN*)leftLeaf, frame);
            if (rightLeaf->type == IDENTIFIER) rightLeaf = lookup_name((TOKEN*)rightLeaf, frame);
            if(leftLeaf->type == mmcINT && rightLeaf->type == mmcINT) {
                return new_int(leftLeaf->v.integer == rightLeaf->v.integer);
            }
            return NULL;
        case NE_OP:
            leftLeaf = interpret(tree->left, frame);
            rightLeaf = interpret(tree->right, frame);
            if (leftLeaf->type == IDENTIFIER) leftLeaf = lookup_name((TOKEN*)leftLeaf, frame);
            if (rightLeaf->type == IDENTIFIER) rightLeaf = lookup_name((TOKEN*)rightLeaf, frame);
            if(leftLeaf->type == mmcINT && rightLeaf->type == mmcINT) {
                return new_int(leftLeaf->v.integer != rightLeaf->v.integer);
            }
            return NULL;
        case INT:
            return (VALUE*)tree;
        case VOID:
            return NULL;
        case FUNCTION:
            return (VALUE*)tree;
        case APPLY:
            leftLeaf = lexical_call_method((TOKEN*)interpret(tree->left, frame), tree->right, frame);
            if (leftLeaf != NULL && leftLeaf->type == mmcRETURN) return leftLeaf->v.ret;
            else return NULL;
        case LEAF:
            return interpret(tree->left, frame);
        case IF:
            leftLeaf = interpret(tree->left, frame);

            if (leftLeaf == NULL) {
                return leftLeaf;
            } else {
                if(leftLeaf->v.integer) {
                    return tree->right->type == ELSE ? interpret(tree->right->left, frame) : interpret(tree->right, frame);
                } else {
                    return tree->right->type == ELSE ? interpret(tree->right->right, frame) : NULL;
                }
            }
        case ELSE:
            return NULL;
        case WHILE:
            while (interpret(tree->left, frame)->v.integer) {
                VALUE *ret = interpret(tree->right, frame);
                if (ret != NULL) {
                    switch (ret->type) {
                        case BREAK:
                            break;
                        case mmcRETURN:
                            return ret;
                    }
                }
            }
            return NULL;
        case CONTINUE:
            return (VALUE*)tree;
        case BREAK:
            return (VALUE*)tree;
        case RETURN:
            leftLeaf = interpret(tree->left, frame);
            if (leftLeaf->type == IDENTIFIER) {
                return new_return(lookup_name((TOKEN*)leftLeaf, frame));
            } else return new_return(leftLeaf);
    }
}