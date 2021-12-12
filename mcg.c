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

    sprintf(insn, "entry_%s:", i->args.proc.name->lexeme);
    this = new_mci(insn);
    prev->next = this;
    prev = this;

    insn = (char*)malloc(50 * sizeof(char));
    sprintf(insn, "li $v0, 9");
    this = new_mci(insn);
    prev->next = this;
    prev = this;

    insn = (char*)malloc(50 * sizeof(char));
    sprintf(insn, "li $a0, %d", 4 * (3 + numArgs + numLocals));
    this = new_mci(insn);
    prev->next = this;
    prev = this;

    insn = (char*)malloc(50 * sizeof(char));
    sprintf(insn, "syscall");
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
    sprintf(insn, "sw $a1, 8($v0)");
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
        // a0 is used for sbrk syscall and a1 is used for static link
        if (j < 2) sprintf(insn, "sw $a%d, %d($fp)", j + 2, off);
        else if (j < 12) sprintf(insn, "sw $t%d, %d($fp)", j - 2, off);
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
            sprintf(insn, "li $v0, 9");
            this = new_mci(insn);
            prev->next = this;
            prev = this;

            insn = (char*)malloc(50 * sizeof(char));
            sprintf(insn, "li $a0, 8");
            this = new_mci(insn);
            prev->next = this;
            prev = this;

            insn = (char*)malloc(50 * sizeof(char));
            sprintf(insn, "syscall");
            this = new_mci(insn);
            prev->next = this;
            prev = this;

            insn = (char*)malloc(50 * sizeof(char));
            sprintf(insn, "sw $fp, 0($v0)");
            this = new_mci(insn);
            prev->next = this;
            prev = this;

            insn = (char*)malloc(50 * sizeof(char));
            sprintf(insn, "la $t0, entry_%s", t->lexeme);
            this = new_mci(insn);
            prev->next = this;
            prev = this;

            insn = (char*)malloc(50 * sizeof(char));
            sprintf(insn, "sw $t0, 4($v0)");
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

    prev = mmc_mcg(i->next, this, ar);

    insn = (char*)malloc(50 * sizeof(char));
    sprintf(insn, "lw $fp, 0($fp)");
    this = new_mci(insn);
    prev->next = this;
    prev = this;

    insn = (char*)malloc(50 * sizeof(char));
    sprintf(insn, "jr $ra");
    this = new_mci(insn);
    prev->next = this;
    return this;
}

MC *mcg_premain(TAC *i, MC *p)
{
    MC *prev;
    MC *this;
    TOKEN *m;

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
    sprintf(insn, "main:");
    this = new_mci(insn);
    first = this;
    prev = this;

    insn = (char*)malloc(50 * sizeof(char));
    sprintf(insn, "li $v0, 9");
    this = new_mci(insn);
    prev->next = this;
    prev = this;

    insn = (char*)malloc(50 * sizeof(char));
    sprintf(insn, "li $a0, %d", 4 * (globalAR->localCount));
    this = new_mci(insn);
    prev->next = this;
    prev = this;

    insn = (char*)malloc(50 * sizeof(char));
    sprintf(insn, "syscall");
    this = new_mci(insn);
    prev->next = this;
    prev = this;

    insn = (char*)malloc(50 * sizeof(char));
    sprintf(insn, "move $fp, $v0");
    this = new_mci(insn);
    prev->next = this;
    prev = this;

    insn = (char*)malloc(50 * sizeof(char));
    sprintf(insn, "move $gp, $v0");
    this = new_mci(insn);
    prev->next = this;
    prev = this;

    for (int j = 0; j < globalAR->localCount; j++) {
        TOKEN *t = globalAR->local[j];
        t->value = j * 4;
        if (find_list(procs, t)) {
            if (strcmp(t->lexeme, "main") == 0) m = t;

            insn = (char*)malloc(50 * sizeof(char));
            sprintf(insn, "li $v0, 9");
            this = new_mci(insn);
            prev->next = this;
            prev = this;

            insn = (char*)malloc(50 * sizeof(char));
            sprintf(insn, "li $a0, 8");
            this = new_mci(insn);
            prev->next = this;
            prev = this;

            insn = (char*)malloc(50 * sizeof(char));
            sprintf(insn, "syscall");
            this = new_mci(insn);
            prev->next = this;
            prev = this;

            insn = (char*)malloc(50 * sizeof(char));
            sprintf(insn, "sw $fp, 0($v0)");
            this = new_mci(insn);
            prev->next = this;
            prev = this;

            insn = (char*)malloc(50 * sizeof(char));
            sprintf(insn, "la $t0, entry_%s", t->lexeme);
            this = new_mci(insn);
            prev->next = this;
            prev = this;

            insn = (char*)malloc(50 * sizeof(char));
            sprintf(insn, "sw $t0, 4($v0)");
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
                sprintf(insn, "lw $%s, %d($fp)", curr->args.line.dst->lexeme, curr->args.line.src1->value);
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

    insn = (char*)malloc(50 * sizeof(char));
    sprintf(insn, "lw $a1 %d($fp)", m->value);
    this = new_mci(insn);
    prev->next = this;
    prev = this;

    insn = (char*)malloc(50 * sizeof(char));
    sprintf(insn, "lw $a0 4($a1)");
    this = new_mci(insn);
    prev->next = this;
    prev = this;

    insn = (char*)malloc(50 * sizeof(char));
    sprintf(insn, "lw $a1 0($a1)");
    this = new_mci(insn);
    prev->next = this;
    prev = this;

    insn = (char*)malloc(50 * sizeof(char));
    sprintf(insn, "j $a0");
    this = new_mci(insn);
    prev->next = this;
    prev = this;

    for (TAC *curr = i; curr != NULL; curr = curr->next) {
        if (curr->op == tac_proc) {
            this = mcg_compute_proc(curr, this);
        }
    }

    return first;
}

MC *mcg_compute_if(TAC *i, MC *prev) // Fix for dynamic allocation
{
    MC *this;
    MC *next;
    TAC *cond = i->args.tacif.cond;
    char *label = i->args.tacif.label->name->lexeme;
    char *insn = (char*)malloc(50 * sizeof(char));

    switch (cond->op) {
        case tac_eq:
            sprintf(insn, "beq $%s, $%s, %s", cond->args.line.src1->lexeme, cond->args.line.src2->lexeme, label);
            this = new_mci(insn);
            prev->next = this;
            return this;
        case tac_ne:
            sprintf(insn, "bne $%s, $%s, %s", cond->args.line.src1->lexeme, cond->args.line.src2->lexeme, label);
            this = new_mci(insn);
            prev->next = this;
            return this;
        case tac_ge:
//            sprintf(insn, "slt $at $%s, $%s", cond->args.line.src1->lexeme, cond->args.line.src2->lexeme);
//            this = new_mci(insn);
//            prev->next = this;
//            insn = (char*)malloc(50 * sizeof(char));
//            sprintf(insn, "beq $at $zero %s", label);
//            next = new_mci(insn);
//            this->next = next;
//            return next;
            sprintf(insn, "bge $%s, $%s, %s", cond->args.line.src1->lexeme, cond->args.line.src2->lexeme, label);
            this = new_mci(insn);
            prev->next = this;
            return this;
        case tac_gt:
//            sprintf(insn, "slt $at $%s, $%s", cond->args.line.src2->lexeme, cond->args.line.src1->lexeme);
//            this = new_mci(insn);
//            prev->next = this;
//            insn = (char*)malloc(50 * sizeof(char));
//            sprintf(insn, "bne $at $zero %s", label);
//            next = new_mci(insn);
//            this->next = next;
//            return next;
            sprintf(insn, "bgt $%s, $%s, %s", cond->args.line.src1->lexeme, cond->args.line.src2->lexeme, label);
            this = new_mci(insn);
            prev->next = this;
            return this;
        case tac_le:
//            sprintf(insn, "slt $at $%s, $%s", cond->args.line.src2->lexeme, cond->args.line.src1->lexeme);
//            this = new_mci(insn);
//            prev->next = this;
//            insn = (char*)malloc(50 * sizeof(char));
//            sprintf(insn, "beq $at $zero %s", label);
//            next = new_mci(insn);
//            this->next = next;
//            return next;
            sprintf(insn, "ble $%s, $%s, %s", cond->args.line.src1->lexeme, cond->args.line.src2->lexeme, label);
            this = new_mci(insn);
            prev->next = this;
            return this;
        case tac_lt:
//            sprintf(insn, "slt $at $%s, $%s", cond->args.line.src1->lexeme, cond->args.line.src2->lexeme);
//            this = new_mci(insn);
//            prev->next = this;
//            insn = (char*)malloc(50 * sizeof(char));
//            sprintf(insn, "bne $at $zero %s", label);
//            next = new_mci(insn);
//            this->next = next;
//            return next;
            sprintf(insn, "blt $%s, $%s, %s", cond->args.line.src1->lexeme, cond->args.line.src2->lexeme, label);
            this = new_mci(insn);
            prev->next = this;
            return this;
        default:
            return NULL;
    }
}

LLIST *find_ref_rec(TOKEN *t, AR *ar)
{
    int pos = -1;

    for (int i = 0; i < ar->arity; i++) {
        if (ar->param[i] == t) {
            pos = t->value;
        }
    }
    for (int i = 0; i < ar->localCount; i++) {
        if (ar->local[i] == t) {
            pos = t->value;
        }
    }

    if (pos == -1) {
        if (ar->sl == NULL) {
            printf("Error, token not found.\n");
            exit(EXIT_FAILURE);
        }

        pos = 8;
        LLIST *ret = new_llist(&pos);

        return join_llist(ret, find_ref_rec(t, ar->sl));
    } else {
        return new_llist(&pos);
    }
}

ILLIST *find_ref(TOKEN *t, AR *ar)
{
    int pos = -1;

    for (int i = 0; i < ar->arity; i++) {
        if (ar->param[i] == t) {
            pos = t->value;
        }
    }

    for (int i = 0; i < ar->localCount; i++) {
        if (ar->local[i] == t) {
            pos = t->value;
        }
    }

    if (pos == -1) {
        if (ar->sl == NULL) {
            printf("Error, token not found.\n");
            exit(EXIT_FAILURE);
        }

        pos = 8;
        ILLIST *ret = new_illist(pos);

        return join_illist(ret, find_ref(t, ar->sl));
    } else {
        return new_illist(pos);
    }
}

MC* mmc_mcg(TAC* i, MC *p, AR *ar)
{
    if (i==NULL) return p;
    MC *prev = p;

    MC *this;
    char *insn = (char*)malloc(50 * sizeof(char));

    ILLIST *ref;
    char *reg;

    switch (i->op) {
        case tac_load_const:
            printf("load_const\n");
            sprintf(insn, "li $%s, %d", i->args.line.dst->lexeme, i->args.line.src1->value);
            this = new_mci(insn);
            break;
        case tac_load_id:
            printf("load_id\n");
            ref = find_ref(i->args.line.src1, ar);
            reg = i->args.line.dst->lexeme;
            sprintf(insn, "lw $%s, %d($fp)", reg, ref->i);
            this = new_mci(insn);
            prev->next = this;
            prev = this;
            while (ref->next != NULL) {
                ref = ref->next;

                insn = (char*)malloc(50 * sizeof(char));
                sprintf(insn, "lw $%s, %d($%s)", reg, ref->i, reg);
                this = new_mci(insn);
                prev->next = this;
                prev = this;
            }
            return mmc_mcg(i->next, this, ar);
        case tac_store:
            printf("store\n");
            ref = find_ref(i->args.line.dst, ar);
            if (count_ilist(ref) == 1) {
                sprintf(insn, "sw $%s, %d($fp)", i->args.line.src1->lexeme, ref->i);
                this = new_mci(insn);
            } else {
                sprintf(insn, "lw $a0, %d($fp)", ref->i);
                this = new_mci(insn);
                prev->next = this;
                prev = this;
                while (ref->next != NULL) {
                    ref = ref->next;

                    insn = (char *) malloc(50 * sizeof(char));
                    sprintf(insn, "lw $a0, %d($a0)", ref->i);
                    this = new_mci(insn);
                    prev->next = this;
                    prev = this;
                }
                insn = (char *) malloc(50 * sizeof(char));
                sprintf(insn, "sw $%s, $a0", i->args.line.src1->lexeme);
                this = new_mci(insn);
            }
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
                prev->next = this;
                prev = this;
            } else if (i->args.line.src1->type == IDENTIFIER) {
                ref = find_ref(i->args.line.src1, ar);

                sprintf(insn, "lw $v0, %d($fp)", ref->i);
                this = new_mci(insn);
                prev->next = this;
                prev = this;
                while (ref->next != NULL) {
                    ref = ref->next;

                    insn = (char*)malloc(50 * sizeof(char));
                    sprintf(insn, "lw $v0, %d($v0)", ref->i);
                    this = new_mci(insn);
                    prev->next = this;
                    prev = this;
                }
            } else {
                sprintf(insn, "move $v0, $%s", i->args.line.src1->lexeme);
                this = new_mci(insn);
                prev->next = this;
                prev = this;
            }

            insn = (char*)malloc(50 * sizeof(char));
            sprintf(insn, "lw $fp, 0($fp)");
            this = new_mci(insn);
            prev->next = this;
            prev = this;

            insn = (char*)malloc(50 * sizeof(char));
            sprintf(insn, "jr $ra");
            this = new_mci(insn);
            break;
        case tac_proc:
            printf("Error in compilation.\n");
            exit(EXIT_FAILURE);
        case tac_endproc:
            return prev;
        case tac_call:
            printf("call\n");
            ref = find_ref(i->args.call.name, ar);
            int arity = i->args.call.ar->arity;
            TOKEN **args = i->args.call.ar->param;
            for (int j = 0; j < arity; j++) {
                char *argReg = (char*)malloc(5 * sizeof(char));
                if (j < 2) sprintf(argReg, "$a%d", j + 2);
                else if (j < 12) sprintf(argReg, "$t%d", j - 2);
                else {
                    printf("Too many arguments.\n");
                    exit(EXIT_FAILURE);
                }
                if (args[j]->type == CONSTANT) {
                    insn = (char *) malloc(50 * sizeof(char));
                    sprintf(insn, "li %s, %d", argReg, args[j]->value);
                    this = new_mci(insn);
                    prev->next = this;
                    prev = this;
                } else if (args[j]->type == IDENTIFIER) {
                    ILLIST *argRef = find_ref(args[j], ar);

                    insn = (char*)malloc(50 * sizeof(char));
                    sprintf(insn, "lw %s, %d($fp)", argReg, argRef->i);
                    this = new_mci(insn);
                    prev->next = this;
                    prev = this;

                    while (argRef->next != NULL) {
                        argRef = argRef->next;

                        insn = (char*)malloc(50 * sizeof(char));
                        sprintf(insn, "lw %s, %d(%s)", argReg, argRef->i, argReg);
                        this = new_mci(insn);
                        prev->next = this;
                        prev = this;
                    }
                } else {
                    printf("Unknown argument type.\n");
                    exit(EXIT_FAILURE);
                }
            }

            insn = (char*)malloc(50 * sizeof(char));
            sprintf(insn, "lw $a1, %d($fp)", ref->i);
            this = new_mci(insn);
            prev->next = this;
            prev = this;
            while (ref->next != NULL) {
                ref = ref->next;

                insn = (char*)malloc(50 * sizeof(char));
                sprintf(insn, "lw $a1, %d($a1)", ref->i);
                this = new_mci(insn);
                prev->next = this;
                prev = this;
            }

            insn = (char*)malloc(50 * sizeof(char));
            sprintf(insn, "lw $a0, 4($a1)");
            this = new_mci(insn);
            prev->next = this;
            prev = this;

            insn = (char*)malloc(50 * sizeof(char));
            sprintf(insn, "lw $a1, 0($a1)");
            this = new_mci(insn);
            prev->next = this;
            prev = this;

            insn = (char*)malloc(50 * sizeof(char));
            sprintf(insn, "jal $a0");
            this = new_mci(insn);
            prev->next = this;
            prev = this;

            insn = (char*)malloc(50 * sizeof(char));
            sprintf(insn, "lw $ra, 4($fp)");
            this = new_mci(insn);
            break;
        case tac_label:
            printf("label\n");
            sprintf(insn, "%s:\t", i->args.taclabel.name->lexeme);
            this = new_mci(insn);
            break;
        case tac_if:
            return mmc_mcg(i->next, mcg_compute_if(i, prev), ar);
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

void limit_temps(TAC *tac)
{
    LLIST *temps = new_llist(NULL);

    for (TAC *curr = tac; curr != NULL; curr = curr->next) {
        switch (curr->op) {
            case tac_load_const:
            case tac_load_id:
            case tac_mod:
            case tac_times:
            case tac_plus:
            case tac_sub:
            case tac_div:
                if (curr->args.line.dst->type == TEMPORARY) {
                    if (!find_list(temps, curr->args.line.dst))
                        append_llist(temps, curr->args.line.dst);
                }
                break;
            case tac_if:
                if (curr->args.tacif.cond->args.line.dst->type == TEMPORARY) {
                    if (!find_list(temps, curr->args.tacif.cond->args.line.dst))
                        append_llist(temps, curr->args.tacif.cond->args.line.dst);
                }
                break;
            case tac_store:
            case tac_return:
            case tac_proc:
            case tac_endproc:
            case tac_call:
            case tac_label:
            case tac_goto:
            default:
                break;
        }
    }

    if (temps->next == NULL) return;
    else temps = temps->next;

    int count = count_list(temps);
    TOKEN **tokens = (TOKEN**)malloc(count * sizeof(TOKEN*));
    LLIST *curr = temps;
    int i = 0;
    while (curr != NULL) {
        tokens[i++] = (TOKEN*)curr->item;
        curr = curr->next;
    }

    int reg = 0;
    for (int j = (count - 1); j >= 0; j--) {
        TOKEN *tok = tokens[j];
        if (tok->lexeme[0] == 't') {
            free(tok->lexeme);
            tok->lexeme = (char*)malloc(sizeof(char));
            sprintf(tok->lexeme, "t%d", reg);
            reg = (reg + 1) % 10;
        }
    }
}

//int limit_temps(LLIST *temps)
//{
//    int reg;
//    if (temps->next != NULL) reg = limit_temps(temps->next);
//    else reg = 0;
//
//    for (LLIST *curr = temps; curr != NULL; curr = curr->next) {
//        if (tac->args.line.dst->lexeme[0] == 't') {
//            free(tac->args.line.dst->lexeme);
//            tac->args.line.dst->lexeme = (char*)malloc(sizeof(char));
//            sprintf(tac->args.line.dst->lexeme, "$t%d", reg);
//            reg = (reg + 1) % 10;
//        }
//    }
//}

void limit_temps_bb(BB *bb)
{
    if (bb->next != NULL) limit_temps_bb(bb->next);
    limit_temps(bb->leader);
}

MC *mmc_mcg_bb(BB *bb)
{
    limit_temps_bb(bb);

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
    while(t != NULL && (t->op != tac_proc || strcmp(t->args.proc.name->lexeme, "main") != 0))
        t = t->next;

    TAC *main = t;
//    t->next = NULL;

    globalAR = main->args.proc.ar->sl;
    // Need to do register stuff here
    printf("here\n");
    mcg_premain(tq->start, NULL);
//    mmc_mcg(main, NULL);

    return first;
}