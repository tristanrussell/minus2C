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
            printf("%s\n", t->lexeme);
            val = declare_name(t, frame, type);
    }

    return val;
}

VALUE *create_closure(NODE *tree, FRAME *frame)
{
    if (tree->type != 'D') return NULL;

    NODE *dec = tree->left;
    NODE *code = tree->right;

    if (dec->type != 'd' || code == NULL) {
        printf("Error in function declaration.");
        exit(EXIT_FAILURE);
    }

    VALUE *type = interpret(dec->left, frame);

    NODE *func = dec->right;

    if (func->type != 'F') {
        printf("Error in function declaration.");
        exit(EXIT_FAILURE);
    }

    TOKEN *name = (TOKEN*)interpret(func->left, frame);
    NODE *ids = func->right;

    if (name == NULL || ids == NULL) {
        printf("Error in function declaration.");
        exit(EXIT_FAILURE);
    }

    if (strcmp(name->lexeme, "main") == 0) {
        FRAME *newFrame = extend_frame(frame, NULL, NULL);
        newFrame->next = frame;
        return interpret(code, newFrame);
    } else return declare_closure(name, frame, code, ids, type->type);
}

VALUE *lexical_call_method(TOKEN *name, NODE *args, FRAME *env)
{
    VALUE *v = lookup_name(name, env);
    if (v == NULL || v->type != mmcCLOSURE) {
        if (strcmp(name->lexeme, "print_int") == 0) return print_int(args, env);
        else if (strcmp(name->lexeme, "read_int") == 0) return read_int();
        printf("Non-existent function called.");
        exit(EXIT_FAILURE);
    } else {
        printf("Found:%s\n", name->lexeme);
    }
    CLOSURE *f = v->v.closure;
    FRAME *newenv = extend_frame(env, f->ids, args);
    newenv->next = f->env;
    return interpret(f->code, newenv);
}

VALUE *print_int(NODE *args, FRAME *frame)
{
    VALUE *arg = interpret(args, frame);
    if (arg->type == IDENTIFIER) {
        arg = lookup_name((TOKEN*)arg, frame);
    }

    printf("%d\n", arg->v.integer);

    return NULL;
}

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

VALUE *premain(NODE *tree, FRAME *frame)
{
    // Do stuff

    return interpret(tree, frame);
}

VALUE *interpret(NODE *tree, FRAME *frame)
{
    TOKEN *t = (TOKEN *)tree;
    VALUE *leftLeaf;
    VALUE *rightLeaf;

    switch (tree->type) {
        default: printf("???\n"); return NULL;
        case '%':
            printf("%%\n");
            leftLeaf = interpret(tree->left, frame);
            rightLeaf = interpret(tree->right, frame);
            if (leftLeaf->type == IDENTIFIER) leftLeaf = lookup_name((TOKEN*)leftLeaf, frame);
            if (rightLeaf->type == IDENTIFIER) rightLeaf = lookup_name((TOKEN*)rightLeaf, frame);
            if (leftLeaf->type == mmcINT && rightLeaf->type == mmcINT) {
                return new_int(leftLeaf->v.integer % rightLeaf->v.integer);
            }
            return NULL;
        case '*':
            printf("*\n");
            leftLeaf = interpret(tree->left, frame);
            rightLeaf = interpret(tree->right, frame);
            if (leftLeaf->type == IDENTIFIER) leftLeaf = lookup_name((TOKEN*)leftLeaf, frame);
            if (rightLeaf->type == IDENTIFIER) rightLeaf = lookup_name((TOKEN*)rightLeaf, frame);
            if (leftLeaf->type == mmcINT && rightLeaf->type == mmcINT) {
                return new_int(leftLeaf->v.integer * rightLeaf->v.integer);
            }
            return NULL;
        case '+':
            printf("+\n");
            leftLeaf = interpret(tree->left, frame);
            rightLeaf = interpret(tree->right, frame);
            if (leftLeaf->type == IDENTIFIER) leftLeaf = lookup_name((TOKEN*)leftLeaf, frame);
            if (rightLeaf->type == IDENTIFIER) rightLeaf = lookup_name((TOKEN*)rightLeaf, frame);
            if (leftLeaf->type == mmcINT && rightLeaf->type == mmcINT) {
                return new_int(leftLeaf->v.integer + rightLeaf->v.integer);
            }
            return NULL;
        case '-':
            printf("-\n");
            leftLeaf = interpret(tree->left, frame);
            rightLeaf = interpret(tree->right, frame);
            if (leftLeaf->type == IDENTIFIER) leftLeaf = lookup_name((TOKEN*)leftLeaf, frame);
            if (rightLeaf->type == IDENTIFIER) rightLeaf = lookup_name((TOKEN*)rightLeaf, frame);
            if (leftLeaf->type == mmcINT && rightLeaf->type == mmcINT) {
                return new_int(leftLeaf->v.integer - rightLeaf->v.integer);
            }
            return NULL;
        case '/':
            printf("/\n");
            leftLeaf = interpret(tree->left, frame);
            rightLeaf = interpret(tree->right, frame);
            if (leftLeaf->type == IDENTIFIER) leftLeaf = lookup_name((TOKEN*)leftLeaf, frame);
            if (rightLeaf->type == IDENTIFIER) rightLeaf = lookup_name((TOKEN*)rightLeaf, frame);
            if (leftLeaf->type == mmcINT && rightLeaf->type == mmcINT) {
                return new_int(leftLeaf->v.integer / rightLeaf->v.integer);
            }
            return NULL;
        case ';':
            printf(";\n");
            leftLeaf = interpret(tree->left, frame);
            if (leftLeaf != NULL && leftLeaf->type == CONTINUE) return leftLeaf;
            return interpret(tree->right, frame);
        case '<':
            printf("<\n");
            leftLeaf = interpret(tree->left, frame);
            rightLeaf = interpret(tree->right, frame);
            if (leftLeaf->type == IDENTIFIER) leftLeaf = lookup_name((TOKEN*)leftLeaf, frame);
            if (rightLeaf->type == IDENTIFIER) rightLeaf = lookup_name((TOKEN*)rightLeaf, frame);
            if(leftLeaf->type == mmcINT && rightLeaf->type == mmcINT) {
                return new_int(leftLeaf->v.integer < rightLeaf->v.integer);
            }
            return NULL;
        case '=':
            printf("=\n");
            leftLeaf = interpret(tree->left, frame);
            rightLeaf = interpret(tree->right, frame);
            if (rightLeaf->type == IDENTIFIER) rightLeaf = lookup_name((TOKEN*)rightLeaf, frame);
            assign_name((TOKEN*)leftLeaf, frame, rightLeaf);
            return NULL;
        case '>':
            printf(">\n");
            leftLeaf = interpret(tree->left, frame);
            rightLeaf = interpret(tree->right, frame);
            if (leftLeaf->type == IDENTIFIER) leftLeaf = lookup_name((TOKEN*)leftLeaf, frame);
            if (rightLeaf->type == IDENTIFIER) rightLeaf = lookup_name((TOKEN*)rightLeaf, frame);
            if(leftLeaf->type == mmcINT && rightLeaf->type == mmcINT) {
                return new_int(leftLeaf->v.integer > rightLeaf->v.integer);
            }
            return NULL;
        case 'D':
            printf("D\n");
            return create_closure(tree, frame);
        case 'F':
            printf("F\n");
            return NULL;
        case 'd':
            printf("d\n");
            return NULL;
        case '~':
            printf("~\n");
            if (tree->left->type == '~' || tree->left->type == 'D') {
                leftLeaf = interpret(tree->left, frame);
                if (leftLeaf != NULL && leftLeaf->type == mmcRETURN) return leftLeaf;
                rightLeaf = interpret(tree->right, frame);
                if (rightLeaf != NULL && rightLeaf->type == mmcRETURN) return rightLeaf;
            } else create_vars(tree->right, frame, interpret(tree->left, frame)->type);
            return NULL;
        case IDENTIFIER:
            printf("id\n");
            printf("%s\n", t->lexeme);
            return (VALUE*)tree;
        case CONSTANT:
            printf("const\n");
            return new_int(t->value);
        case STRING_LITERAL:
            printf("string\n");
            return (VALUE*)tree;
        case LE_OP:
            printf("<=\n");
            leftLeaf = interpret(tree->left, frame);
            rightLeaf = interpret(tree->right, frame);
            if (leftLeaf->type == IDENTIFIER) leftLeaf = lookup_name((TOKEN*)leftLeaf, frame);
            if (rightLeaf->type == IDENTIFIER) rightLeaf = lookup_name((TOKEN*)rightLeaf, frame);
            if(leftLeaf->type == mmcINT && rightLeaf->type == mmcINT) {
                return new_int(leftLeaf->v.integer <= rightLeaf->v.integer);
            }
            return NULL;
        case GE_OP:
            printf(">=\n");
            leftLeaf = interpret(tree->left, frame);
            rightLeaf = interpret(tree->right, frame);
            if (leftLeaf->type == IDENTIFIER) leftLeaf = lookup_name((TOKEN*)leftLeaf, frame);
            if (rightLeaf->type == IDENTIFIER) rightLeaf = lookup_name((TOKEN*)rightLeaf, frame);
            if(leftLeaf->type == mmcINT && rightLeaf->type == mmcINT) {
                return new_int(leftLeaf->v.integer >= rightLeaf->v.integer);
            }
            return NULL;
        case EQ_OP:
            printf("==\n");
            leftLeaf = interpret(tree->left, frame);
            rightLeaf = interpret(tree->right, frame);
            if (leftLeaf->type == IDENTIFIER) leftLeaf = lookup_name((TOKEN*)leftLeaf, frame);
            if (rightLeaf->type == IDENTIFIER) rightLeaf = lookup_name((TOKEN*)rightLeaf, frame);
            if(leftLeaf->type == mmcINT && rightLeaf->type == mmcINT) {
                return new_int(leftLeaf->v.integer == rightLeaf->v.integer);
            }
            return NULL;
        case NE_OP:
            printf("!=\n");
            leftLeaf = interpret(tree->left, frame);
            rightLeaf = interpret(tree->right, frame);
            if (leftLeaf->type == IDENTIFIER) leftLeaf = lookup_name((TOKEN*)leftLeaf, frame);
            if (rightLeaf->type == IDENTIFIER) rightLeaf = lookup_name((TOKEN*)rightLeaf, frame);
            if(leftLeaf->type == mmcINT && rightLeaf->type == mmcINT) {
                return new_int(leftLeaf->v.integer != rightLeaf->v.integer);
            }
            return NULL;
        case EXTERN:
            printf("extern\n");
            return NULL;
        case AUTO:
            printf("auto\n");
            return NULL;
        case INT:
            printf("int\n");
            return (VALUE*)tree;
        case VOID:
            printf("void\n");
            return NULL;
        case FUNCTION:
            printf("function\n");
            return (VALUE*)tree;
        case APPLY:
            printf("apply\n");
            leftLeaf = lexical_call_method((TOKEN*)interpret(tree->left, frame), tree->right, frame);
            if (leftLeaf != NULL && leftLeaf->type == mmcRETURN) return leftLeaf->v.ret;
            else return NULL;
        case LEAF:
            printf("leaf\n");
            return interpret(tree->left, frame);
        case IF:
            printf("if\n");
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
            printf("else\n");
            return NULL;
        case WHILE:
            printf("while\n");
            while (interpret(tree->left, frame)->v.integer) {
                VALUE *ret = interpret(tree->right, frame);
                if (ret != NULL && ret->type == BREAK) break;
            }
            return NULL;
        case CONTINUE:
            printf("continue\n");
            return (VALUE*)tree;
        case BREAK:
            printf("break\n");
            return (VALUE*)tree;
        case RETURN:
            printf("return\n");
            leftLeaf = interpret(tree->left, frame);
            if (leftLeaf->type == IDENTIFIER) {
                return new_return(lookup_name((TOKEN*)leftLeaf, frame));
            } else return new_return(leftLeaf);
    }
}