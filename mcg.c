#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "mcg.h"
#include "tac.h"
#include "mc.h"
#include "llist.h"
#include "C.tab.h"

// A pointer to the first MIPS instruction.
MC *first = NULL;
// A pointer to the global activation record.
AR *globalAR = NULL;

// A linked list of procedures found in the TAC sequence.
LLIST *procs = NULL;

MC *mmc_mcg(TAC*, MC*, AR*);
ILLIST *find_ref(TOKEN *t, AR *ar);

/**
 * Handles creating the code for a procedure. It generates the code for the
 * frame allocation and the allocation of procedure's parameters and then it
 * calls the function to generate the procedure's code.
 *
 * @param i : The TAC instruction at the start of the procedure.
 * @param p : The previous MIPS instruction that the new instructions will be
 *            appended to.
 * @return : The last instruction generated for the procedure.
 */
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

/**
 * This function is the entry point for the TAC to machine code compilation. It
 * builds the premain code as well as calling functions to build the code for
 * main and the other functions. It takes in a sequence of TAC instructions and
 * returns the compiled MIPS machine code.
 *
 * @param i : The sequence of TAC instructions to compile.
 * @return : The compiled sequence of MIPS machine code instructions.
 */
MC *mcg_premain(TAC *i)
{
    MC *prev;
    MC *this;
    TOKEN *m;

    for (TAC *a = i; a != NULL; a = a->next) {
        if (a->op == tac_proc) {
            if (procs == NULL) {
                procs = new_llist(a->args.proc.name);
            } else {
                append_llist(procs, a->args.proc.name);
            }
        }
    }

    char *insn = (char*)malloc(50 * sizeof(char));
    sprintf(insn, ".text");
    this = new_mci(insn);
    first = this;
    prev = this;

    insn = (char*)malloc(50 * sizeof(char));
    sprintf(insn, ".globl main");
    this = new_mci(insn);
    prev->next = this;
    prev = this;

    insn = (char*)malloc(50 * sizeof(char));
    sprintf(insn, "main:");
    this = new_mci(insn);
    prev->next = this;
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
                insn = (char*)malloc(50 * sizeof(char));
                sprintf(insn, "li $%s, %d", curr->args.line.dst->lexeme, curr->args.line.src1->value);
                this = new_mci(insn);
                prev->next = this;
                prev = this;
                break;
            case tac_load_id:
                insn = (char*)malloc(50 * sizeof(char));
                sprintf(insn, "lw $%s, %d($fp)", curr->args.line.dst->lexeme, curr->args.line.src1->value);
                this = new_mci(insn);
                prev->next = this;
                prev = this;
                break;
            case tac_store:
                insn = (char*)malloc(50 * sizeof(char));
                sprintf(insn, "sw $%s, %d($fp)", curr->args.line.src1->lexeme, curr->args.line.dst->value);
                this = new_mci(insn);
                prev->next = this;
                prev = this;
                break;
            case tac_mod:
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
                insn = (char*)malloc(50 * sizeof(char));
                sprintf(insn, "mul $%s, $%s, $%s", curr->args.line.dst->lexeme, curr->args.line.src1->lexeme, curr->args.line.src2->lexeme);
                this = new_mci(insn);
                prev->next = this;
                prev = this;
                break;
            case tac_plus:
                insn = (char*)malloc(50 * sizeof(char));
                sprintf(insn, "add $%s, $%s, $%s", curr->args.line.dst->lexeme, curr->args.line.src1->lexeme, curr->args.line.src2->lexeme);
                this = new_mci(insn);
                prev->next = this;
                prev = this;
                break;
            case tac_sub:
                insn = (char*)malloc(50 * sizeof(char));
                sprintf(insn, "sub $%s, $%s, $%s", curr->args.line.dst->lexeme, curr->args.line.src1->lexeme, curr->args.line.src2->lexeme);
                this = new_mci(insn);
                prev->next = this;
                prev = this;
                break;
            case tac_div:
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

/**
 * Takes a TAC CALL instruction and checks if it is a built in function, if it
 * is then it returns the sequence of instructions for that function, otherwise
 * NULL is returned.
 *
 * @param i : The TAC CALL instruction.
 * @param p : THe previous MIPS machine code instruction that any new
 *            instructions will be appended to.
 * @param ar : The activation record for the frame that we are currently in.
 * @return : The sequence of instructions for the built in function or NULL.
 */
MC *check_built_in(TAC *i, MC *p, AR *ar)
{
    MC *this;
    MC *prev = p;
    char *name = i->args.call.name->lexeme;
    int arity;
    TOKEN **args;
    char *insn = (char*)malloc(50 * sizeof(char));

    if (strcmp(name, "print_int") == 0) {
        args = i->args.call.ar->param;

        sprintf(insn, "li $v0, 1");
        this = new_mci(insn);
        prev->next = this;
        prev = this;

        if (args[0]->type == CONSTANT) {
            insn = (char *) malloc(50 * sizeof(char));
            sprintf(insn, "li $a0, %d", args[0]->value);
            this = new_mci(insn);
            prev->next = this;
            prev = this;
        } else if (args[0]->type == IDENTIFIER) {
            ILLIST *argRef = find_ref(args[0], ar);

            insn = (char*)malloc(50 * sizeof(char));
            sprintf(insn, "lw $a0, %d($fp)", argRef->i);
            this = new_mci(insn);
            prev->next = this;
            prev = this;

            while (argRef->next != NULL) {
                argRef = argRef->next;

                insn = (char*)malloc(50 * sizeof(char));
                sprintf(insn, "lw $a0, %d($a0)", argRef->i);
                this = new_mci(insn);
                prev->next = this;
                prev = this;
            }
        } else {
            printf("Unknown argument type.\n");
            exit(EXIT_FAILURE);
        }

        insn = (char*)malloc(50 * sizeof(char));
        sprintf(insn, "syscall");
        this = new_mci(insn);
        prev->next = this;
        return this;
    } else if (strcmp(name, "read_int") == 0) {
        sprintf(insn, "li $v0, 5");
        this = new_mci(insn);
        prev->next = this;
        prev = this;

        insn = (char*)malloc(50 * sizeof(char));
        sprintf(insn, "syscall");
        this = new_mci(insn);
        prev->next = this;
        return this;
    } else return NULL;
}

/**
 * Handles the compiling of TAC if statements.
 *
 * @param i : The TAC if instruction.
 * @param prev : The previous MIPS machine code instruction to append any new
 *               instructions onto.
 * @return : The new compiled instruction.
 */
MC *mcg_compute_if(TAC *i, MC *prev)
{
    MC *this;
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
            sprintf(insn, "bge $%s, $%s, %s", cond->args.line.src1->lexeme, cond->args.line.src2->lexeme, label);
            this = new_mci(insn);
            prev->next = this;
            return this;
        case tac_gt:
            sprintf(insn, "bgt $%s, $%s, %s", cond->args.line.src1->lexeme, cond->args.line.src2->lexeme, label);
            this = new_mci(insn);
            prev->next = this;
            return this;
        case tac_le:
            sprintf(insn, "ble $%s, $%s, %s", cond->args.line.src1->lexeme, cond->args.line.src2->lexeme, label);
            this = new_mci(insn);
            prev->next = this;
            return this;
        case tac_lt:
            sprintf(insn, "blt $%s, $%s, %s", cond->args.line.src1->lexeme, cond->args.line.src2->lexeme, label);
            this = new_mci(insn);
            prev->next = this;
            return this;
        default:
            return NULL;
    }
}

/**
 * Finds a variable by recursively checking the current frame for the variable
 * then moving to the parent frame and checking again. This terminates at the
 * global frame, if not found then the program will terminate with an error.
 *
 * The function returns a list of offsets to access to find the variable. If
 * the variable is not in a frame then 8 will be used as that is the offset to
 * access the static link to reach the next frame.
 *
 * @param t : The token to find in the frame.
 * @param ar : The activation record for the current frame, this stores the
 *             tokens for the current frame.
 * @return : A sequence of integers indicating the offsets to find the
 *           variable.
 */
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

/**
 * The function that is called recursively to construct the body of a
 * procedure. It will terminate at an endproc TAC instruction and return the
 * sequence of MIPS machine code instructions to the calling function.
 *
 * @param i : The TAC instruction starting the body of code for a procedure.
 * @param p : The previous MIPS machine code instruction to append new
 *            instructions to.
 * @param ar : The activation record for the current frame.
 * @return : The sequence of compiled MIPS machine code instructions.
 */
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
            sprintf(insn, "li $%s, %d", i->args.line.dst->lexeme, i->args.line.src1->value);
            this = new_mci(insn);
            break;
        case tac_load_id:
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
            sprintf(insn, "div $%s, $%s", i->args.line.src1->lexeme, i->args.line.src2->lexeme);
            this = new_mci(insn);
            prev->next = this;
            prev = this;
            insn = (char*)malloc(50 * sizeof(char));
            sprintf(insn, "mfhi $%s", i->args.line.dst->lexeme);
            this = new_mci(insn);
            break;
        case tac_times:
            sprintf(insn, "mul $%s, $%s, $%s", i->args.line.dst->lexeme, i->args.line.src1->lexeme, i->args.line.src2->lexeme);
            this = new_mci(insn);
            break;
        case tac_plus:
            sprintf(insn, "add $%s, $%s, $%s", i->args.line.dst->lexeme, i->args.line.src1->lexeme, i->args.line.src2->lexeme);
            this = new_mci(insn);
            break;
        case tac_sub:
            sprintf(insn, "sub $%s, $%s, $%s", i->args.line.dst->lexeme, i->args.line.src1->lexeme, i->args.line.src2->lexeme);
            this = new_mci(insn);
            break;
        case tac_div:
            sprintf(insn, "div $%s, $%s", i->args.line.src1->lexeme, i->args.line.src2->lexeme);
            this = new_mci(insn);
            prev->next = this;
            prev = this;
            insn = (char*)malloc(50 * sizeof(char));
            sprintf(insn, "mflo $%s", i->args.line.dst->lexeme);
            this = new_mci(insn);
            break;
        case tac_return:
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
        case tac_call: {
            MC *checkBuiltIn = check_built_in(i, prev, ar);
            if (checkBuiltIn != NULL)
                return mmc_mcg(i->next, checkBuiltIn, ar);

            ref = find_ref(i->args.call.name, ar);
            int arity = i->args.call.ar->arity;
            TOKEN **args = i->args.call.ar->param;
            for (int j = 0; j < arity; j++) {
                char *argReg = (char *) malloc(5 * sizeof(char));
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

                    insn = (char *) malloc(50 * sizeof(char));
                    sprintf(insn, "lw %s, %d($fp)", argReg, argRef->i);
                    this = new_mci(insn);
                    prev->next = this;
                    prev = this;

                    while (argRef->next != NULL) {
                        argRef = argRef->next;

                        insn = (char *) malloc(50 * sizeof(char));
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

            insn = (char *) malloc(50 * sizeof(char));
            sprintf(insn, "lw $a1, %d($fp)", ref->i);
            this = new_mci(insn);
            prev->next = this;
            prev = this;
            while (ref->next != NULL) {
                ref = ref->next;

                insn = (char *) malloc(50 * sizeof(char));
                sprintf(insn, "lw $a1, %d($a1)", ref->i);
                this = new_mci(insn);
                prev->next = this;
                prev = this;
            }

            insn = (char *) malloc(50 * sizeof(char));
            sprintf(insn, "lw $a0, 4($a1)");
            this = new_mci(insn);
            prev->next = this;
            prev = this;

            insn = (char *) malloc(50 * sizeof(char));
            sprintf(insn, "lw $a1, 0($a1)");
            this = new_mci(insn);
            prev->next = this;
            prev = this;

            insn = (char *) malloc(50 * sizeof(char));
            sprintf(insn, "jal $a0");
            this = new_mci(insn);
            prev->next = this;
            prev = this;

            insn = (char *) malloc(50 * sizeof(char));
            sprintf(insn, "lw $ra, 4($fp)");
            this = new_mci(insn);
            break;
        }
        case tac_label:
            sprintf(insn, "%s:\t", i->args.taclabel.name->lexeme);
            this = new_mci(insn);
            break;
        case tac_if:
            return mmc_mcg(i->next, mcg_compute_if(i, prev), ar);
        case tac_goto:
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

/**
 * A simple queue structure for TAC instructions.
 */
typedef struct tqueue {
    TAC *start;
    TAC *end;
} TQUEUE;

/**
 * A simple constructor for a queue of TAC instructions.
 *
 * @param tac : The first TAC instruction to start the queue.
 * @return : The new queue structure.
 */
TQUEUE *tqueue_create(TAC *tac)
{
    TQUEUE *tq = (TQUEUE*)malloc(sizeof(TQUEUE));
    tq->start = tac;
    tq->end = tac;
    return tq;
}

/**
 * Reverses a sequence of TAC instructions using recursion.
 *
 * @param tac : The starting TAC instruction for the sequence of TAC
 *              instructions.
 * @return : A queue of TAC instructions.
 */
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

/**
 * Finds all of the temporaries in a TAC sequence and limits them to fit inside
 * of the MIPS temporary registers.
 *
 * @param tac : The sequence of TAC instructions.
 */
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

/**
 * Limits the temporaries in every basic block to fit into the 10 temporary
 * registers of the MIPS architecture.
 *
 * @param bb : The sequence of basic blocks.
 */
void limit_temps_bb(BB *bb)
{
    if (bb->next != NULL) limit_temps_bb(bb->next);
    limit_temps(bb->leader);
}

/**
 * The entry point for the compiler code. Reverses the TAC instructions, limits
 * the temporary values and finds the global activation record, then calls the
 * entry function for the compilation of TAC to MIPS machine code.
 *
 * @param bb : The basic block sequence of TAC instructions to be compiled.
 * @return : The compiled MIPS machine code.
 */
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

    while(t != NULL && (t->op != tac_proc || strcmp(t->args.proc.name->lexeme, "main") != 0))
        t = t->next;

    TAC *main = t;

    globalAR = main->args.proc.ar->sl;
    mcg_premain(tq->start);

    return first;
}