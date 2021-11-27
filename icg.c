#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "icg.h"
#include "tac.h"
#include "C.tab.h"
#include "token.h"

int countArgs(NODE *ast)
{
    switch (ast->type) {
        case ',':
            return countArgs(ast->left) + countArgs(ast->right);
        case '~':
            return 1;
        default:
            return 0;
    }
}

int countLocals(NODE *ast)
{
    switch (ast->type) {
        case ';':
            return countLocals(ast->left) + countLocals(ast->right);
        case ',':
            if (ast->left->type == ',') return 1 + countLocals(ast->left);
            return 2;
        case '~':
            if (ast->right->type == ',') return countLocals(ast->right);
            return 1;
        default:
            return 0;
    }
}

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
        int countThen = countLocals(ast->right->left);
        TAC *blockThenStart = new_block_tac(countThen);
        TAC *blockThenEnd = new_blockend_tac();

        TAC *elseLeaf = mmc_icg(ast->right->right);
        int countElse = countLocals(ast->right->right);
        TAC *blockElseStart = new_block_tac(countElse);
        TAC *blockElseEnd = new_blockend_tac();

        TAC *labelElse = new_label_tac();
        TAC *labelEnd = new_label_tac();

        TAC *ifTac = new_if_tac(leftLeaf, &labelElse->args.taclabel);
        ifTac->next = leftLeaf;
        blockThenStart->next = ifTac;
        prepend_tac(blockThenStart, thenLeaf);

        TAC *gotoEnd = new_goto_tac(&labelEnd->args.taclabel);
        gotoEnd->next = thenLeaf;
        blockThenEnd->next = gotoEnd;

        labelElse->next = blockThenEnd;
        blockElseStart->next = labelElse;

        prepend_tac(blockElseStart, elseLeaf);
        blockElseEnd->next = elseLeaf;

        labelEnd->next = blockElseEnd;
        return labelEnd;
    } else {
        TAC *rightLeaf = mmc_icg(ast->right);
        int count = countLocals(ast->right);
        TAC *blockStart = new_block_tac(count);
        TAC *blockEnd = new_blockend_tac();

        TAC *label = new_label_tac();

        TAC *ifTac = new_if_tac(leftLeaf, &label->args.taclabel);
        ifTac->next = leftLeaf;
        blockStart->next = ifTac;
        prepend_tac(blockStart, rightLeaf);

        blockEnd->next = rightLeaf;
        label->next = blockEnd;
        return label;
    }
}

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

    int count = countLocals(ast->right);
    TAC *blockStart = new_block_tac(count);
    TAC *blockEnd = new_blockend_tac();
    printf("here\n");

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
    blockStart->next = ifTac;
    prepend_tac(blockStart, rightLeaf);

    gotoTac->next = rightLeaf;
    blockEnd->next = gotoTac;
    labelEnd->next = blockEnd;
    return labelEnd;
}

TAC *tac_compute_closure(NODE *ast)
{
    if (ast->type != 'D') return NULL;

    NODE *dec = ast->left;

    int count = countLocals(ast->right);
    TAC *blockStart = new_block_tac(count);
    TAC *blockEnd = new_blockend_tac();

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
    TAC *endproc = new_procend_tac(proc);

    blockStart->next = proc;

    prepend_tac(blockStart, code);

    blockEnd->next = code;
    endproc->next = blockEnd;

    return endproc;
}

TAC *mmc_icg(NODE* ast)
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

void remove_blocks(TAC *tac)
{
    int count = 0;

    TAC *curr = tac;

    while(curr->op != tac_endproc && curr->next != NULL) curr = curr->next;

    if (curr->next == NULL) return;

    TAC *prev = curr;
    curr = curr->next;
    for (; curr != NULL; prev = curr, curr = curr->next) {
        if (curr->op == tac_block) {
            count += curr->args.block.nvars;
            prev->next = curr->next;
        } else if (curr->op == tac_endblock) {
            prev->next = curr->next;
        } else if (curr->op == tac_endproc) {
            remove_blocks(curr);
            curr = curr->args.endproc.start;
        } else if (curr->op == tac_proc) {
            curr->args.proc.locals = count;
            if (curr->next != NULL) remove_blocks(curr->next);
            return;
        }
    }
}

TAC *remove_post_main(TAC *tac)
{
    TAC *curr = tac;
    while(curr != NULL && (curr->op != tac_endproc || strcmp(curr->args.endproc.start->args.proc.name->lexeme, "main") != 0)) curr = curr->next;
    return curr;
}

TAC *tac_optimise(TAC *tac)
{
    remove_blocks(tac);
    return remove_post_main(tac);
}

BB *bb_create(TAC* seq)
{
    BB *bb;
    if (seq->next == NULL) {
        bb = (BB*)malloc(sizeof(BB));
        return bb;
    } else bb = bb_create(seq->next);

    int jumpOps[] = {tac_proc, tac_label};
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