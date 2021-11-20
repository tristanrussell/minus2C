#include <stdio.h>
#include <stdlib.h>
#include "mcg.h"
#include "tac.h"
#include "mc.h"

MC *mcg_compute_if(TAC *i, MC *prev)
{
    MC *this;
    MC *next;
    char *label = i->args.tacif.label->name->lexeme;
    char *insn = (char*)malloc(50 * sizeof(char));

    switch (i->args.tacif.cond->op) {
        case tac_eq:
            sprintf(insn, "beq $%s, $%s, %s", i->args.tacif.cond->args.line.src1->lexeme, i->args.tacif.cond->args.line.src2->lexeme, label);
            this = new_mci(insn);
            this->next = prev;
            return this;
        case tac_ne:
            sprintf(insn, "bne $%s, $%s, %s", i->args.tacif.cond->args.line.src1->lexeme, i->args.tacif.cond->args.line.src2->lexeme, label);
            this = new_mci(insn);
            this->next = prev;
            return this;
        case tac_ge:
            sprintf(insn, "slt $at $%s, $%s", i->args.line.src1->lexeme, i->args.line.src2->lexeme);
            this = new_mci(insn);
            this->next = prev;
            insn = (char*)malloc(50 * sizeof(char));
            sprintf(insn, "beq $at $zero %s", label);
            next = new_mci(insn);
            next->next = this;
            return next;
        case tac_gt:
            sprintf(insn, "slt $at $%s, $%s", i->args.line.src2->lexeme, i->args.line.src1->lexeme);
            this = new_mci(insn);
            this->next = prev;
            insn = (char*)malloc(50 * sizeof(char));
            sprintf(insn, "bne $at $zero %s", label);
            next = new_mci(insn);
            next->next = this;
            return next;
        case tac_le:
            sprintf(insn, "slt $at $%s, $%s", i->args.line.src2->lexeme, i->args.line.src1->lexeme);
            this = new_mci(insn);
            this->next = prev;
            insn = (char*)malloc(50 * sizeof(char));
            sprintf(insn, "beq $at $zero %s", label);
            next = new_mci(insn);
            next->next = this;
            return next;
        case tac_lt:
            sprintf(insn, "slt $at $%s, $%s", i->args.line.src1->lexeme, i->args.line.src2->lexeme);
            this = new_mci(insn);
            this->next = prev;
            insn = (char*)malloc(50 * sizeof(char));
            sprintf(insn, "bne $at $zero %s", label);
            next = new_mci(insn);
            next->next = this;
            return next;
        default:
            return NULL;
    }
}

MC* mmc_mcg(TAC* i)
{
    if (i==NULL) return NULL;
    MC *prev = mmc_mcg(i->next);

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
            this->next = prev;
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
            this->next = prev;
            prev = this;
            insn = (char*)malloc(50 * sizeof(char));
            sprintf(insn, "mflo $%s", i->args.line.dst->lexeme);
            this = new_mci(insn);
            break;
//        case tac_return:
//            if (i->args.line.src1->type == CONSTANT)
//                printf("%s %d\n",
//                       tac_ops[i->op],
//                       i->args.line.src1->value);
//            else
//                printf("%s %s\n",
//                       tac_ops[i->op],
//                       i->args.line.src1->lexeme);
//            break;
        case tac_label:
            printf("label\n");
            sprintf(insn, "%s:\t", i->args.taclabel.name->lexeme);
            this = new_mci(insn);
            break;
        case tac_if:
            return mcg_compute_if(i, prev);
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
            return prev;
        default:
            printf("unknown type code %d (%p) in mmc_mcg\n",i->op,i);
            return prev;
    }

    this->next = prev;
    return this;
}

MC *mmc_mcg_bb(BB *bb)
{
    MC *next = NULL;
    if (bb->next != NULL) next = mmc_mcg_bb(bb->next);

    MC *this = mmc_mcg(bb->leader);

    MC *n = this;
    while (n->next != NULL) n = n->next;
    n->next = next;

    return this;
}