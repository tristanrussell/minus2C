#include <stdio.h>
#include <stdlib.h>
#include "icg.h"
#include "tac.h"
#include "C.tab.h"
#include "token.h"


// Need to add blocks
TAC *tac_compute_if(NODE *ast)
{
    TOKEN *temp;

    TAC *leftLeaf = mmc_icg(ast->left);
    switch (leftLeaf->op) {
        case tac_eq:
            leftLeaf->op = tac_ne;
            break;
        case tac_ne:
            leftLeaf->op = tac_eq;
            break;
        case tac_gt:
            leftLeaf->op = tac_le;
            break;
        case tac_ge:
            leftLeaf->op = tac_lt;
            break;
        case tac_lt:
            leftLeaf->op = tac_ge;
            break;
        case tac_le:
            leftLeaf->op = tac_gt;
            break;
        default:
            temp = new_token(CONSTANT);
            temp->value = 0;
            TAC *load = new_line_tac(tac_load_const, temp, NULL, new_temp());
            TAC *comp = new_line_tac(tac_eq, leftLeaf->args.line.dst, load->args.line.dst, new_temp());
            comp->next = load;
            load->next = leftLeaf;
            leftLeaf = comp;
    }

    if (ast->right->type == ELSE) {
        TAC *thenLeaf = mmc_icg(ast->right->left);
        TAC *elseLeaf = mmc_icg(ast->right->right);
        TAC *labelElse = new_label_tac();
        TAC *labelEnd = new_label_tac();
        TAC *ifTac = new_if_tac(leftLeaf, &labelElse->args.taclabel);
        ifTac->next = leftLeaf;
        prepend_tac(ifTac, thenLeaf);
        TAC *gotoEnd = new_goto_tac(&labelEnd->args.taclabel);
        gotoEnd->next = thenLeaf;
        labelElse->next = gotoEnd;
        prepend_tac(labelElse, elseLeaf);
        labelEnd->next = elseLeaf;
        return labelEnd;
    } else {
        TAC *rightLeaf = mmc_icg(ast->right);
        TAC *label = new_label_tac();
        TAC *ifTac = new_if_tac(leftLeaf, &label->args.taclabel);
        ifTac->next = leftLeaf;
        prepend_tac(ifTac, rightLeaf);
        label->next = rightLeaf;
        return label;
    }
}

// Need to add blocks
TAC *tac_compute_while(NODE *ast)
{
    TOKEN *temp;

    TAC *leftLeaf = mmc_icg(ast->left);
    switch (leftLeaf->op) {
        case tac_eq:
            leftLeaf->op = tac_ne;
            break;
        case tac_ne:
            leftLeaf->op = tac_eq;
            break;
        case tac_gt:
            leftLeaf->op = tac_le;
            break;
        case tac_ge:
            leftLeaf->op = tac_lt;
            break;
        case tac_lt:
            leftLeaf->op = tac_ge;
            break;
        case tac_le:
            leftLeaf->op = tac_gt;
            break;
        default:
            temp = new_token(CONSTANT);
            temp->value = 0;
            TAC *load = new_line_tac(tac_load_const, temp, NULL, new_temp());
            TAC *comp = new_line_tac(tac_eq, leftLeaf->args.line.dst, load->args.line.dst, new_temp());
            comp->next = load;
            load->next = leftLeaf;
            leftLeaf = comp;
    }

    TAC *labelStart = new_label_tac();
    TAC *rightLeaf = mmc_icg(ast->right);
    TAC *labelEnd = new_label_tac();
    TAC *ifTac = new_if_tac(leftLeaf, &labelEnd->args.taclabel);
    TAC *gotoTac = new_goto_tac(&labelStart->args.taclabel);

    TAC *r = rightLeaf;
    while (r != NULL) {
        if (r->op == tac_continue) {
            r->op = tac_goto;
            r->args.tacgoto.label = &labelStart->args.taclabel;
        } else if (r->op == tac_break) {
            r->op = tac_goto;
            r->args.tacgoto.label = &labelEnd->args.taclabel;
        }

        r = r->next;
    }

    prepend_tac(labelStart, leftLeaf);
    ifTac->next = leftLeaf;
    prepend_tac(ifTac, rightLeaf);
    gotoTac->next = rightLeaf;
    labelEnd->next = gotoTac;
    return labelEnd;
}

int countArgs(NODE *ast)
{
    int count = 0;
    switch (ast->type) {
        case ',':
            count += countArgs(ast->left);
            count += countArgs(ast->right);
            return count;
        case '~':
            return 1;
        default:
            return 0;
    }
}

TAC *tac_compute_closure(NODE *ast)
{
    if (ast->type != 'D') return NULL;

    NODE *dec = ast->left;
    TAC *code = mmc_icg(ast->right);

    if (dec->type != 'd' || code == NULL) {
        printf("Error in function declaration.");
        exit(EXIT_FAILURE);
    }

//    TAC *type = mmc_icg(dec->left);

    NODE *func = dec->right;

    if (func->type != 'F') {
        printf("Error in function declaration.");
        exit(EXIT_FAILURE);
    }

    TAC *name = mmc_icg(func->left);

    int numArgs = countArgs(func->right);

    TAC *proc = new_proc_tac(name->args.line.src1, numArgs);
    TAC *endproc = new_procend_tac();

    prepend_tac(proc, code);
    endproc->next = code;

    return endproc;
}

TAC* mmc_icg(NODE* ast)
{
    TOKEN *t = (TOKEN*)ast;
    TAC *leftLeaf = NULL;
    TAC *rightLeaf = NULL;
    TAC *ret = NULL;

    switch (ast->type) {
        case '%':
            printf("%%\n");
            leftLeaf = mmc_icg(ast->left);
            rightLeaf = mmc_icg(ast->right);
            ret = new_line_tac(tac_mod, leftLeaf->args.line.dst, rightLeaf->args.line.dst, new_temp());
            prepend_tac(leftLeaf, rightLeaf);
            ret->next = rightLeaf;
            return ret;
        case '*':
            printf("*\n");
            leftLeaf = mmc_icg(ast->left);
            rightLeaf = mmc_icg(ast->right);
            ret = new_line_tac(tac_times, leftLeaf->args.line.dst, rightLeaf->args.line.dst, new_temp());
            prepend_tac(leftLeaf, rightLeaf);
            ret->next = rightLeaf;
            return ret;
        case '+':
            printf("+\n");
            leftLeaf = mmc_icg(ast->left);
            rightLeaf = mmc_icg(ast->right);
            ret = new_line_tac(tac_plus, leftLeaf->args.line.dst, rightLeaf->args.line.dst, new_temp());
            prepend_tac(leftLeaf, rightLeaf);
            ret->next = rightLeaf;
            return ret;
        case '-':
            printf("-\n");
            leftLeaf = mmc_icg(ast->left);
            rightLeaf = mmc_icg(ast->right);
            ret = new_line_tac(tac_sub, leftLeaf->args.line.dst, rightLeaf->args.line.dst, new_temp());
            prepend_tac(leftLeaf, rightLeaf);
            ret->next = rightLeaf;
            return ret;
        case '/':
            printf("/\n");
            leftLeaf = mmc_icg(ast->left);
            rightLeaf = mmc_icg(ast->right);
            ret = new_line_tac(tac_div, leftLeaf->args.line.dst, rightLeaf->args.line.dst, new_temp());
            prepend_tac(leftLeaf, rightLeaf);
            ret->next = rightLeaf;
            return ret;
        case ';':
            printf(";\n");
            leftLeaf = mmc_icg(ast->left);
            rightLeaf = mmc_icg(ast->right);
            return prepend_tac(leftLeaf, rightLeaf);
        case '<':
            printf("<\n");
            leftLeaf = mmc_icg(ast->left);
            rightLeaf = mmc_icg(ast->right);
            prepend_tac(leftLeaf, rightLeaf);
            ret = new_line_tac(tac_lt, leftLeaf->args.line.dst, rightLeaf->args.line.dst, new_temp());
            ret->next = rightLeaf;
            return ret;
        case '=':
            printf("=\n");
            leftLeaf = mmc_icg(ast->left);
            rightLeaf = mmc_icg(ast->right);
//            assign_name((TOKEN*)leftLeaf, frame, rightLeaf);
//            if (rightLeaf->op == tac_load_id) {
//                return new_line_tac(tac_store, rightLeaf->args.line.src1, NULL, leftLeaf->args.line.src1);
//            } else {
            ret = new_line_tac(tac_store, rightLeaf->args.line.dst, NULL, leftLeaf->args.line.src1);
            ret->next = rightLeaf;
            return ret;
//            }
        case '>':
            printf(">\n");
            leftLeaf = mmc_icg(ast->left);
            rightLeaf = mmc_icg(ast->right);
            prepend_tac(leftLeaf, rightLeaf);
            ret = new_line_tac(tac_gt, leftLeaf->args.line.dst, rightLeaf->args.line.dst, new_temp());
            ret->next = rightLeaf;
            return ret;
        case 'D':
            printf("D\n");
            return tac_compute_closure(ast);
        case '~':
            printf("~\n");
            leftLeaf = mmc_icg(ast->left);
            rightLeaf = mmc_icg(ast->right);
            if (leftLeaf != NULL && rightLeaf != NULL) {
                prepend_tac(leftLeaf, rightLeaf);
                return rightLeaf;
            } else if (leftLeaf != NULL) return leftLeaf;
            else if (rightLeaf != NULL) return rightLeaf;
            else return NULL;
//            create_vars(ast->right, frame, ast->left->left->type);
            return NULL;
        case IDENTIFIER:
            printf("id\n");
            return new_line_tac(tac_load_id, t, NULL, new_temp());
        case CONSTANT:
            printf("const\n");
            return new_line_tac(tac_load_const, t, NULL, new_temp());
        case LE_OP:
            printf("<=\n");
            leftLeaf = mmc_icg(ast->left);
            rightLeaf = mmc_icg(ast->right);
            prepend_tac(leftLeaf, rightLeaf);
            ret = new_line_tac(tac_le, leftLeaf->args.line.dst, rightLeaf->args.line.dst, new_temp());
            ret->next = rightLeaf;
            return ret;
        case GE_OP:
            printf(">=\n");
            leftLeaf = mmc_icg(ast->left);
            rightLeaf = mmc_icg(ast->right);
            prepend_tac(leftLeaf, rightLeaf);
            ret = new_line_tac(tac_ge, leftLeaf->args.line.dst, rightLeaf->args.line.dst, new_temp());
            ret->next = rightLeaf;
            return ret;
        case EQ_OP:
            printf("==\n");
            leftLeaf = mmc_icg(ast->left);
            rightLeaf = mmc_icg(ast->right);
            prepend_tac(leftLeaf, rightLeaf);
            ret = new_line_tac(tac_eq, leftLeaf->args.line.dst, rightLeaf->args.line.dst, new_temp());
            ret->next = rightLeaf;
            return ret;
        case NE_OP:
            printf("!=\n");
            leftLeaf = mmc_icg(ast->left);
            rightLeaf = mmc_icg(ast->right);
            prepend_tac(leftLeaf, rightLeaf);
            ret = new_line_tac(tac_ne, leftLeaf->args.line.dst, rightLeaf->args.line.dst, new_temp());
            ret->next = rightLeaf;
            return ret;
        case INT:
            printf("int\n");
            return NULL;
        case VOID:
            printf("void\n");
            return NULL;
        case LEAF:
            printf("leaf\n");
            return mmc_icg(ast->left);
        case IF:
            printf("if\n");
            return tac_compute_if(ast);
        case ELSE:
            printf("else\n");
            return NULL;
        case WHILE:
            printf("while\n");
            // label L(x)
            // if (cond) goto L(y)
            // continue loop code
            // Look at asm examples for inspiration.
            return tac_compute_while(ast);
        case CONTINUE:
            printf("continue\n");
            // This should be goto L(x), label at start of while loop.
            return new_line_tac(tac_continue, NULL, NULL, NULL);
        case BREAK:
            printf("break\n");
            // This should be goto L(y), label just after while loop.
            return new_line_tac(tac_break, NULL, NULL, NULL);
        case RETURN:
            printf("return\n");
            leftLeaf = mmc_icg(ast->left);
            if (leftLeaf->op == tac_load_id || leftLeaf->op == tac_load_const) {
                return new_line_tac(tac_return, leftLeaf->args.line.src1, NULL, NULL);
            } else {
                ret = new_line_tac(tac_return, leftLeaf->args.line.dst, NULL, NULL);
                ret->next = leftLeaf;
                return ret;
            }
        default:
            printf("unknown type code %d (%p) in mmc_icg\n",ast->type,ast);
            return NULL;
    }
}

BB* bb_create(TAC* seq)
{
    BB *bb;
    if (seq->next == NULL) {
        bb = (BB*)malloc(sizeof(BB));
        return bb;
    } else bb = bb_create(seq->next);

    int jumpOps[] = {tac_proc, tac_call, tac_block, tac_label};
    int len = 4;
    for (int i = 0; i < len; i++) {
        if (seq->op == jumpOps[i]) {
            BB *nextBB = (BB*)malloc(sizeof(BB));
            nextBB->next = bb;
            seq->next = NULL;
            nextBB->leader = seq;
            return nextBB;
        }
    }

    bb->leader = seq;
    return bb;
}

BB* bb_optimise(BB* bb)
{
    return (BB*)0;
}

void bb_print(BB* bb)
{
    if (bb->next != NULL) bb_print(bb->next);

    printf("BASIC BLOCK START\n");

    mmc_print_ic(bb->leader, 2);

    printf("BASIC BLOCK END\n");
}