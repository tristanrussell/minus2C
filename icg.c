#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "icg.h"
#include "tac.h"
#include "C.tab.h"
#include "token.h"
#include "llist.h"

static AR *globalAR = NULL;

TOKEN **convert_token_array(LLIST *list)
{
    TOKEN **tokens = (TOKEN**)malloc(count_list(list) * sizeof(TOKEN*));
    LLIST *curr = list;
    int i = 0;
    while (curr != NULL) {
        tokens[i++] = (TOKEN*)curr->item;
        curr = curr->next;
    }
    return tokens;
}

LLIST *getParams(NODE *ast)
{
    switch (ast->type) {
        case ',':
            return join_llist(getParams(ast->left), getParams(ast->right));
        case '~':
            return getParams(ast->right);
        case LEAF:
            return new_llist((void*)ast->left);
        default:
            return NULL;
    }
}

LLIST *getLocals(NODE *ast)
{
    LLIST *left;
    LLIST *right;

    switch (ast->type) {
        case ';':
            left = getLocals(ast->left);
            right = getLocals(ast->right);
            if (left == NULL && right == NULL) return NULL;
            if (left == NULL) return right;
            if (right == NULL) return left;
            return join_llist(left, right);
        case ',':
            return join_llist(getLocals(ast->left), getLocals(ast->right));
        case '~':
            if (ast->right->type == '=') return getLocals(ast->right->left);
            return getLocals(ast->right);
        case 'D':
            return new_llist((void*)ast->left->right->left->left);
        case LEAF:
            return new_llist((void*)ast->left);
        default:
            return NULL;
    }
}

LLIST *getArgs(NODE *ast)
{
    switch(ast->type) {
        case ',':
            return join_llist(getArgs(ast->left), getArgs(ast->right));
        case APPLY:
        case LEAF:
            return new_llist((void*)ast->left);
        default:
            return NULL;
    }
}

LLIST *getGlobals(NODE *ast)
{
    LLIST *left;
    LLIST *right;

    switch (ast->type) {
        case ';':
        case '~':
            left = getGlobals(ast->left);
            right = getGlobals(ast->right);
            if (left == NULL && right == NULL) return NULL;
            if (left == NULL) return right;
            if (right == NULL) return left;
            return join_llist(left, right);
        case '=':
            return getGlobals(ast->left);
        case ',':
            return join_llist(getGlobals(ast->left), getGlobals(ast->right));
        case 'D':
            return new_llist((void*)ast->left->right->left->left);
        case LEAF:
            if (ast->left->type == IDENTIFIER) return new_llist((void*)ast->left);
            else return NULL;
        default:
            return NULL;
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
        LLIST *localsThen = getLocals(ast->right->left);
        int countThen = count_list(localsThen);

        TAC *blockThenStart = new_block_tac(countThen, convert_token_array(localsThen));
        TAC *blockThenEnd = new_blockend_tac();

        TAC *elseLeaf = mmc_icg(ast->right->right);
        LLIST *localsElse = getLocals(ast->right->right);
        int countElse = count_list(localsElse);

        TAC *blockElseStart = new_block_tac(countElse, convert_token_array(localsElse));
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
        LLIST *locals = getLocals(ast->right);
        int count = count_list(locals);

        TAC *blockStart = new_block_tac(count, convert_token_array(locals));
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

    LLIST *locals = getLocals(ast->right);
    int count = count_list(locals);
    TAC *blockStart = new_block_tac(count, convert_token_array(locals));
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

    LLIST *locals = getLocals(ast->right);
    int count = count_list(locals);
    TAC *blockStart = new_block_tac(count, convert_token_array(locals));
    TAC *blockEnd = new_blockend_tac();

    TAC *code = mmc_icg(ast->right);

    if (dec->type != 'd' || code == NULL) {
        printf("Error in function declaration.\n");
        exit(EXIT_FAILURE);
    }

//    TAC *type = mmc_icg(dec->left);

    NODE *func = dec->right;

    if (func->type != 'F') {
        printf("Error in function declaration.\n");
        exit(EXIT_FAILURE);
    }

    TAC *name = mmc_icg(func->left);

    LLIST *params = getParams(func->right);
    int numParams = count_list(params);

    TAC *proc = new_proc_tac(name->args.line.src1, numParams);
    proc->args.proc.ar->param = convert_token_array(params);
    TAC *endproc = new_procend_tac(proc);

    blockStart->next = proc;

    prepend_tac(blockStart, code);

    blockEnd->next = code;
    endproc->next = blockEnd;

    return endproc;
}

typedef struct tlist {
    TAC *tac;
    struct tlist *next;
} TLIST;

TLIST *tac_compute_args(NODE *ast)
{
    TLIST *left;
    TLIST *right;

    switch(ast->type) {
        case ',':
            left = tac_compute_args(ast->left);
            right = tac_compute_args(ast->right);
            right->next = left;
            return right;
        case APPLY:
        case LEAF:
            left = (TLIST *)malloc(sizeof(TLIST));
            left->tac = mmc_icg(ast);
            return left;
        default:
            return NULL;
    }
}

TAC *tac_compute_call(NODE *ast)
{
    TAC *leftLeaf = mmc_icg(ast->left);
    TAC *ret = new_call_tac(leftLeaf->args.line.src1, 0);

    LLIST *args = getArgs(ast->right);
    int numArgs = count_list(args);

//    TLIST *list = tac_compute_args(ast->right);
//    TLIST *curr = list;
//    TLIST *prev = NULL;
//    while (curr != NULL) {
//        countArgs++;
//        if (curr->tac->op == tac_call) {
//            prepend_tac(curr->tac, ret);
//            if (prev != NULL) {
//                prev->next = curr->next;
//            } else {
//                list = curr->next;
//                continue;
//            }
//        }
//        prev = curr;
//        curr = curr->next;
//    }
//    curr = list;
//    while (curr != NULL) {
//        prepend_tac(curr->tac, ret);
//        curr = curr->next;
//    }

    ret->args.call.ar->arity = numArgs;
    ret->args.call.ar->param = convert_token_array(args);
    return ret;
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
            if (rightLeaf->op == tac_call) {
                TOKEN *rv = new_token(TEMPORARY);
                rv->lexeme = "rv";
                ret = new_line_tac(tac_store, rv, NULL, leftLeaf->args.line.src1);
            } else {
                ret = new_line_tac(tac_store, rightLeaf->args.line.dst, NULL, leftLeaf->args.line.src1);
            }
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
        case FUNCTION:
            printf("function\n");
            return NULL;
        case APPLY: // Need to implement.
            printf("apply\n");
            return tac_compute_call(ast);
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
            } else if (leftLeaf->op == tac_call) {
                TOKEN *rv = new_token(TEMPORARY);
                rv->lexeme = "rv";
                ret = new_line_tac(tac_return, rv, NULL, NULL);
                ret->next = leftLeaf;
                return ret;
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

TAC *generate_tac(NODE *tree)
{
    globalAR = (AR*)malloc(sizeof(AR));
    LLIST *locals = getGlobals(tree);
    globalAR->local = convert_token_array(locals);
    globalAR->localCount = count_list(locals);
    return mmc_icg(tree);
}

void remove_blocks(TAC *tac)
{
    int count = 0;

    TAC *curr = tac;
    LLIST *list = new_llist(NULL);

    while (curr->op != tac_endproc && curr->next != NULL) curr = curr->next;

    if (curr->next == NULL) return;

    TAC *prev = curr;
    curr = curr->next;
    for (; curr != NULL; prev = curr, curr = curr->next) {
        if (curr->op == tac_block) {
            count += curr->args.block.nvars;
            for (int i = 0; i < count; i++) {
                list = append_llist(list, curr->args.block.vars[i]);
            }
            prev->next = curr->next;
        } else if (curr->op == tac_endblock) {
            prev->next = curr->next;
        } else if (curr->op == tac_endproc) {
            remove_blocks(curr);
            curr = curr->args.endproc.start;
        } else if (curr->op == tac_proc) {
            curr->args.proc.ar->localCount = count;
            curr->args.proc.ar->local = convert_token_array(list->next);
            if (curr->next != NULL) remove_blocks(curr->next);
            return;
        }
    }
}

void add_static_links(TAC *tac)
{
    TAC *curr = tac;
    AR *ar = globalAR;
    printf("%d\n", globalAR->localCount);

    while (curr->op != tac_endproc && curr->next != NULL) curr = curr->next;

    if (curr->next == NULL) return;

    for (; curr != NULL; curr = curr->next) {
        if (curr->op == tac_endproc) {
            curr->args.endproc.start->args.proc.ar->sl = ar;
            ar = curr->args.endproc.start->args.proc.ar;
        } else if (curr->op == tac_proc) {
            ar = curr->args.proc.ar->sl;
        } else if (curr->op == tac_call) {
            curr->args.call.ar->fp = ar;
        }
    }
}

TAC *flatten_proc_rec(TAC *tac)
{
    TAC *ret = tac;
    TAC *end = tac->args.endproc.start;
    TAC *curr = tac->next;
    TAC *prev = tac;
    for (; curr != NULL; prev = curr, curr = curr->next) {
        if (curr == end) break;
        else if (curr->op == tac_endproc) {
            prev->next = curr->args.endproc.start->next;
            curr->args.endproc.start->next = NULL;
            ret = flatten_proc_rec(curr);
            prepend_tac(tac, ret);
            curr = prev;
        }
    }
    return ret;
}

void flatten_procedures(TAC *tac)
{
    TAC *curr = tac;
    TAC *next = NULL;

    while(curr->op != tac_endproc && curr->next != NULL) curr = curr->next;

    if (curr->next == NULL) return;

    TAC *prev = NULL;
    for (; curr != NULL; prev = curr, curr = curr->next) {
        if (curr->op == tac_endproc) {
            next = curr->args.endproc.start->next;
            curr->args.endproc.start->next = NULL;
            curr = flatten_proc_rec(curr);
            if (prev != NULL) prev->next = curr;
            while (curr->next != NULL) curr = curr->next;
            curr->next = next;
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
    TAC *ret = remove_post_main(tac);
    add_static_links(ret);
    flatten_procedures(ret);
    return ret;
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