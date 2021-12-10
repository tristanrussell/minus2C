#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "mcg.h"
#include "tac.h"
#include "mc.h"
#include "llist.h"
#include "C.tab.h"

MC *first = NULL;
MC *premain = NULL;
AR *globalAR = NULL;

LLIST *procs = NULL;

MC *append_mc(MC *pre, MC *post)
{
    MC *n = post;
    while (n->next != NULL) n = n->next;
    n->next = pre;
    return post;
}

MC *mcg_compute_proc(TAC *i, MC *p)
{
    int numArgs = i->args.proc.ar->arity;
    int numLocals = i->args.proc.ar->localCount;
    AR *ar = i->args.proc.ar;

    MC *prev = p;
    MC *this;
    char *insn = (char*)malloc(50 * sizeof(char));

    if (strcmp(i->args.proc.name->lexeme, "main") == 0) {
        sprintf(insn, "main:");
    }
    else
        sprintf(insn, "entry_%s:", i->args.proc.name->lexeme);
    this = new_mci(insn);
    prev->next = this;
    prev = this;

    insn = (char*)malloc(50 * sizeof(char));
    sprintf(insn, "li $a0, %d", 4 * (3 + numArgs + numLocals));
    this = new_mci(insn);
    prev->next = this;
    prev = this;

    insn = (char*)malloc(50 * sizeof(char));
    sprintf(insn, "syscall sbrk");
    this = new_mci(insn);
    prev->next = this;
    prev = this;

    insn = (char*)malloc(50 * sizeof(char));
    sprintf(insn, "sw $fp, 0($v0)");
    this = new_mci(insn);
    prev->next = this;
    prev = this;

    insn = (char*)malloc(50 * sizeof(char));
    sprintf(insn, "sw $ra, 4($v0)");
    this = new_mci(insn);
    prev->next = this;
    prev = this;

    insn = (char*)malloc(50 * sizeof(char));
    sprintf(insn, "sw $a0, 8($v0)");
    this = new_mci(insn);
    prev->next = this;
    prev = this;

    insn = (char*)malloc(50 * sizeof(char));
    sprintf(insn, "move $fp, $v0");
    this = new_mci(insn);
    prev->next = this;
    prev = this;

    for (int j = 0; j < numArgs; j++) {
        int off = (3 + j) * 4;
        ar->param[j]->value = off;
        insn = (char*)malloc(50 * sizeof(char));
        if (j < 4) sprintf(insn, "sw $a%d, %d($fp)", j, off);
        else if (j < 14) sprintf(insn, "sw $t%d, %d($fp)", j - 4, off);
        else {
            printf("Too many arguments.");
            exit(EXIT_FAILURE);
        }
        this = new_mci(insn);
        prev->next = this;
        prev = this;
    }

    for (int j = 0; j < numLocals; j++) {
        TOKEN *t = ar->local[j];
        t->value = (3 + numArgs + j) * 4;
        if (find_list(procs, t)) {
            insn = (char*)malloc(50 * sizeof(char));
            sprintf(insn, "li $a0, 8");
            this = new_mci(insn);
            prev->next = this;
            prev = this;

            insn = (char*)malloc(50 * sizeof(char));
            sprintf(insn, "syscall sbrk");
            this = new_mci(insn);
            prev->next = this;
            prev = this;

            insn = (char*)malloc(50 * sizeof(char));
            sprintf(insn, "sw $fp, 0($v0)");
            this = new_mci(insn);
            prev->next = this;
            prev = this;

            insn = (char*)malloc(50 * sizeof(char));
            sprintf(insn, "sw entry_%s, 4($v0)", t->lexeme);
            this = new_mci(insn);
            prev->next = this;
            prev = this;

            insn = (char*)malloc(50 * sizeof(char));
            sprintf(insn, "sw $v0, %d($fp)", t->value);
            this = new_mci(insn);
            prev->next = this;
            prev = this;
        }
    }

    return mmc_mcg(i->next, this, ar);
}

MC *mcg_premain(TAC *i, MC *p)
{
    MC *prev;
    MC *this;

    for (TAC *a = i; a != NULL; a = a->next) {
        if (a->op == tac_proc) {
            printf("%s\n", a->args.proc.name->lexeme);
            if (procs == NULL) {
                procs = new_llist(a->args.proc.name);
            } else {
                append_llist(procs, a->args.proc.name);
            }
        }
    }

    char *insn = (char*)malloc(50 * sizeof(char));
    sprintf(insn, "premain:");
    this = new_mci(insn);
    first = this;
    prev = this;

    insn = (char*)malloc(50 * sizeof(char));
    sprintf(insn, "li $a0, %d", 4 * (globalAR->localCount));
    this = new_mci(insn);
    prev->next = this;
    prev = this;

    insn = (char*)malloc(50 * sizeof(char));
    sprintf(insn, "syscall sbrk");
    this = new_mci(insn);
    prev->next = this;
    prev = this;

    insn = (char*)malloc(50 * sizeof(char));
    sprintf(insn, "move $fp, $v0");
    this = new_mci(insn);
    prev->next = this;
    prev = this;

    for (int j = 0; j < globalAR->localCount; j++) {
        TOKEN *t = globalAR->local[j];
        t->value = j * 4;
        if (find_list(procs, t)) {
            insn = (char*)malloc(50 * sizeof(char));
            sprintf(insn, "li $a0, 8");
            this = new_mci(insn);
            prev->next = this;
            prev = this;

            insn = (char*)malloc(50 * sizeof(char));
            sprintf(insn, "syscall sbrk");
            this = new_mci(insn);
            prev->next = this;
            prev = this;

            insn = (char*)malloc(50 * sizeof(char));
            sprintf(insn, "sw $fp, 0($v0)");
            this = new_mci(insn);
            prev->next = this;
            prev = this;

            insn = (char*)malloc(50 * sizeof(char));
            sprintf(insn, "sw entry_%s, 4($v0)", t->lexeme);
            this = new_mci(insn);
            prev->next = this;
            prev = this;

            insn = (char*)malloc(50 * sizeof(char));
            sprintf(insn, "sw $v0, %d($fp)", t->value);
            this = new_mci(insn);
            prev->next = this;
            prev = this;
        }
    }

    // Variable setting in global scope.
    for (TAC *curr = i; curr != NULL; curr = curr->next) {
        switch (curr->op) {
            case tac_load_const:
                printf("load_const\n");
                insn = (char*)malloc(50 * sizeof(char));
                sprintf(insn, "li $%s, %d", curr->args.line.dst->lexeme, curr->args.line.src1->value);
                this = new_mci(insn);
                prev->next = this;
                prev = this;
                break;
            case tac_load_id:
                printf("load_id\n");
                insn = (char*)malloc(50 * sizeof(char));
                sprintf(insn, "lw $%s, %s", curr->args.line.dst->lexeme, curr->args.line.src1->lexeme);
                this = new_mci(insn);
                prev->next = this;
                prev = this;
                break;
            case tac_store:
                printf("store\n");
                insn = (char*)malloc(50 * sizeof(char));
                sprintf(insn, "sw $%s, %d($fp)", curr->args.line.src1->lexeme, curr->args.line.dst->value);
                this = new_mci(insn);
                prev->next = this;
                prev = this;
                break;
            case tac_mod:
                printf("mod\n");
                insn = (char*)malloc(50 * sizeof(char));
                sprintf(insn, "div $%s, $%s", curr->args.line.src1->lexeme, curr->args.line.src2->lexeme);
                this = new_mci(insn);
                prev->next = this;
                prev = this;
                insn = (char*)malloc(50 * sizeof(char));
                sprintf(insn, "mfhi $%s", curr->args.line.dst->lexeme);
                this = new_mci(insn);
                prev->next = this;
                prev = this;
                break;
            case tac_times:
                printf("multiply\n");
                insn = (char*)malloc(50 * sizeof(char));
                sprintf(insn, "mul $%s, $%s, $%s", curr->args.line.dst->lexeme, curr->args.line.src1->lexeme, curr->args.line.src2->lexeme);
                this = new_mci(insn);
                prev->next = this;
                prev = this;
                break;
            case tac_plus:
                printf("add\n");
                insn = (char*)malloc(50 * sizeof(char));
                sprintf(insn, "add $%s, $%s, $%s", curr->args.line.dst->lexeme, curr->args.line.src1->lexeme, curr->args.line.src2->lexeme);
                this = new_mci(insn);
                prev->next = this;
                prev = this;
                break;
            case tac_sub:
                printf("sub\n");
                insn = (char*)malloc(50 * sizeof(char));
                sprintf(insn, "sub $%s, $%s, $%s", curr->args.line.dst->lexeme, curr->args.line.src1->lexeme, curr->args.line.src2->lexeme);
                this = new_mci(insn);
                prev->next = this;
                prev = this;
                break;
            case tac_div:
                printf("div\n");
                insn = (char*)malloc(50 * sizeof(char));
                sprintf(insn, "div $%s, $%s", curr->args.line.src1->lexeme, curr->args.line.src2->lexeme);
                this = new_mci(insn);
                prev->next = this;
                prev = this;
                insn = (char*)malloc(50 * sizeof(char));
                sprintf(insn, "mflo $%s", curr->args.line.dst->lexeme);
                this = new_mci(insn);
                prev->next = this;
                prev = this;
                break;
            case tac_proc:
                while (curr->op != tac_endproc) curr = curr->next;
                break;
        }
    }

    for (TAC *curr = i; curr != NULL; curr = curr->next) {
        if (curr->op == tac_proc) {
            this = mcg_compute_proc(curr, this);
        }
    }

    return first;
}

MC *mcg_compute_if(TAC *i, MC *prev)
{
    MC *this;
    MC *next;
    TAC *cond = i->args.tacif.cond;
    char *label = i->args.tacif.label->name->lexeme;
    char *insn = (char*)malloc(50 * sizeof(char));

    switch (cond->op) {
        case tac_eq:
            sprintf(insn, "beq $%s, $%s, %s", cond->args.line.src1->lexeme, i->args.tacif.cond->args.line.src2->lexeme, label);
            this = new_mci(insn);
            prev->next = this;
            return this;
        case tac_ne:
            sprintf(insn, "bne $%s, $%s, %s", cond->args.line.src1->lexeme, i->args.tacif.cond->args.line.src2->lexeme, label);
            this = new_mci(insn);
            prev->next = this;
            return this;
        case tac_ge:
            sprintf(insn, "slt $at $%s, $%s", i->args.line.src1->lexeme, i->args.line.src2->lexeme);
            this = new_mci(insn);
            prev->next = this;
            insn = (char*)malloc(50 * sizeof(char));
            sprintf(insn, "beq $at $zero %s", label);
            next = new_mci(insn);
            this->next = next;
            return next;
        case tac_gt:
            sprintf(insn, "slt $at $%s, $%s", i->args.line.src2->lexeme, i->args.line.src1->lexeme);
            this = new_mci(insn);
            prev->next = this;
            insn = (char*)malloc(50 * sizeof(char));
            sprintf(insn, "bne $at $zero %s", label);
            next = new_mci(insn);
            this->next = next;
            return next;
        case tac_le:
            sprintf(insn, "slt $at $%s, $%s", i->args.line.src2->lexeme, i->args.line.src1->lexeme);
            this = new_mci(insn);
            prev->next = this;
            insn = (char*)malloc(50 * sizeof(char));
            sprintf(insn, "beq $at $zero %s", label);
            next = new_mci(insn);
            this->next = next;
            return next;
        case tac_lt:
            sprintf(insn, "slt $at $%s, $%s", i->args.line.src1->lexeme, i->args.line.src2->lexeme);
            this = new_mci(insn);
            prev->next = this;
            insn = (char*)malloc(50 * sizeof(char));
            sprintf(insn, "bne $at $zero %s", label);
            next = new_mci(insn);
            this->next = next;
            return next;
        default:
            return NULL;
    }
}

//char *find_ref(TOKEN *t, AR *ar)
//{
//
//}

MC* mmc_mcg(TAC* i, MC *p, AR *ar)
{
    if (i==NULL) return p;
    MC *prev = p;

    MC *this;
    char *insn = (char*)malloc(50 * sizeof(char));

    switch (i->op) {
        case tac_load_const:
            printf("load_const\n");
            sprintf(insn, "li $%s, %d", i->args.line.dst->lexeme, i->args.line.src1->value);
            this = new_mci(insn);
            break;
        case tac_load_id:
            printf("load_id\n");
            sprintf(insn, "lw $%s, %s", i->args.line.dst->lexeme, i->args.line.src1->lexeme);
            this = new_mci(insn);
            break;
        case tac_store:
            printf("store\n");
            sprintf(insn, "sw $%s, %s", i->args.line.src1->lexeme, i->args.line.dst->lexeme);
            this = new_mci(insn);
            break;
        case tac_mod:
            printf("mod\n");
            sprintf(insn, "div $%s, $%s", i->args.line.src1->lexeme, i->args.line.src2->lexeme);
            this = new_mci(insn);
            prev->next = this;
            prev = this;
            insn = (char*)malloc(50 * sizeof(char));
            sprintf(insn, "mfhi $%s", i->args.line.dst->lexeme);
            this = new_mci(insn);
            break;
        case tac_times:
            printf("multiply\n");
            sprintf(insn, "mul $%s, $%s, $%s", i->args.line.dst->lexeme, i->args.line.src1->lexeme, i->args.line.src2->lexeme);
            this = new_mci(insn);
            break;
        case tac_plus:
            printf("add\n");
            sprintf(insn, "add $%s, $%s, $%s", i->args.line.dst->lexeme, i->args.line.src1->lexeme, i->args.line.src2->lexeme);
            this = new_mci(insn);
            break;
        case tac_sub:
            printf("sub\n");
            sprintf(insn, "sub $%s, $%s, $%s", i->args.line.dst->lexeme, i->args.line.src1->lexeme, i->args.line.src2->lexeme);
            this = new_mci(insn);
            break;
        case tac_div:
            printf("div\n");
            sprintf(insn, "div $%s, $%s", i->args.line.src1->lexeme, i->args.line.src2->lexeme);
            this = new_mci(insn);
            prev->next = this;
            prev = this;
            insn = (char*)malloc(50 * sizeof(char));
            sprintf(insn, "mflo $%s", i->args.line.dst->lexeme);
            this = new_mci(insn);
            break;
        case tac_return:
            printf("return\n");
            if (i->args.line.src1->type == CONSTANT) {
                sprintf(insn, "li $v0, %d", i->args.line.src1->value);
                this = new_mci(insn);
            } else if (i->args.line.src1->type == IDENTIFIER) {
//                sprintf(insn, "lw $v0, %d", i->args.line.src1);
//                this = new_mci(insn);
//                prev->next = this;
//                prev = this;
            } else {
                //
            }
            break;
        case tac_proc:
            return mmc_mcg(i->next, prev, ar);
        case tac_endproc:
            return prev;
        case tac_label:
            printf("label\n");
            sprintf(insn, "%s:\t", i->args.taclabel.name->lexeme);
            this = new_mci(insn);
            break;
        case tac_if:
            this = mcg_compute_if(i, prev);
            break;
        case tac_goto:
            printf("goto\n");
            sprintf(insn, "j %s", i->args.tacgoto.label->name->lexeme);
            this = new_mci(insn);
            break;
        case tac_eq:
        case tac_ne:
        case tac_ge:
        case tac_gt:
        case tac_le:
        case tac_lt:
            return mmc_mcg(i->next, prev, ar);
        default:
            printf("unknown type code %d (%p) in mmc_mcg\n",i->op,i);
            return prev;
    }

    if (prev != NULL) prev->next = this;
    return mmc_mcg(i->next, this, ar);
}

typedef struct tqueue {
    TAC *start;
    TAC *end;
} TQUEUE;

TQUEUE *tqueue_create(TAC *tac)
{
    TQUEUE *tq = (TQUEUE*)malloc(sizeof(TQUEUE));
    tq->start = tac;
    tq->end = tac;
    return tq;
}

TQUEUE *reverse_tac(TAC *tac)
{
    if (tac->next == NULL) return tqueue_create(tac);
    else {
        TQUEUE *tq = reverse_tac(tac->next);
        tq->end->next = tac;
        tq->end = tac;
        return tq;
    }
}

MC *mmc_mcg_bb(BB *bb)
{
    BB *prev = bb;
    BB *curr = bb->next;
    for (; curr != NULL; prev = curr, curr = curr->next) {
        prepend_tac(curr->leader, prev->leader);
    }

    TQUEUE *tq = reverse_tac(bb->leader);
    tq->end->next = NULL;

    TAC *t = tq->start;
    if (t == NULL) {
        printf("Critical error.\n");
        exit(EXIT_FAILURE);
    }
//    if (t->op == tac_proc && strcmp(t->args.proc.name->lexeme, "main") == 0) {
//        mmc_mcg(tq->start, NULL);
//        return first;
//    }
    while(t->next != NULL && (t->next->op != tac_proc || strcmp(t->next->args.proc.name->lexeme, "main") != 0))
        t = t->next;

    TAC *main = t->next;
//    t->next = NULL;

    globalAR = main->args.proc.ar->sl;
    // Need to do register stuff here

    mcg_premain(tq->start, NULL);
//    mmc_mcg(main, NULL);

    return first;
}