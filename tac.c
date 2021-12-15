#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "tac.h"
#include "token.h"
#include "C.tab.h"

// The current temporary count.
int reg_count = 0;
// The current label count.
int label_count = 1;

char* tac_ops[] = {"NOOP","LOAD","LOAD","STORE","MOD","MULTIPLY","ADD","SUBTRACT","DIVIDE","RETURN","PROC","ENDPROC","BLOCK","ENDBLOCK","CALL","LABEL","IF","GOTO","EQ","NE","GTEQ","GT","LTEQ","LT","BREAK","CONTINUE"};

/**
 * Creates a new TAC label with the next available label. Labels are in the
 * format L1, L2, etc.
 *
 * @return : The TAC label.
 */
TAC *new_label_tac()
{
    TOKEN *t = new_token(LABEL);
    char *labelName = (char*)malloc(10 * sizeof(char));
    sprintf(labelName, "L%d", label_count);
    t->lexeme = labelName;
    label_count++;
    TAC *tac = (TAC*)malloc(sizeof(TAC));
    tac->op = tac_label;
    tac->args.taclabel.name = t;
    return tac;
}

/**
 * Creates a new TAC if instruction from a condition and a label. The label is
 * the label to jump to if the test passes.
 *
 * @param cond : The if condition.
 * @param label : The label to jump to.
 * @return : The new TAC if instruction.
 */
TAC *new_if_tac(TAC *cond, TACLABEL *label)
{
    TAC *ifTac = (TAC*)malloc(sizeof(TAC));
    ifTac->op = tac_if;
    ifTac->args.tacif.cond = cond;
    ifTac->args.tacif.label = label;
    return ifTac;
}

/**
 * Creates a new TAC goto instruction to a given label.
 *
 * @param label : The label to jump to.
 * @return : The new TAC goto instruction.
 */
TAC *new_goto_tac(TACLABEL *label)
{
    TAC *gotoTac = (TAC*)malloc(sizeof(TAC));
    gotoTac->op = tac_goto;
    gotoTac->args.tacgoto.label = label;
    return gotoTac;
}

/**
 * Creates a new TAC procedure instruction.
 *
 * @param name : The token name of the procedure.
 * @param arity : The number of parameters for the procedure.
 * @return : The new TAC procedure instruction.
 */
TAC *new_proc_tac(TOKEN *name, int arity)
{
    TAC *procTac = (TAC*)malloc(sizeof(TAC));
    procTac->op = tac_proc;
    procTac->args.proc.name = name;
    procTac->args.proc.ar = (AR*)malloc(sizeof(AR));
    procTac->args.proc.ar->arity = arity;
    procTac->args.proc.ar->localCount = 0;
    return procTac;
}

/**
 * Creates a new TAC procedure end instruction.
 *
 * @param start : The TAC procedure instruction marking the start of the
 *                procedure.
 * @return : The new TAC procedure end instruction.
 */
TAC *new_procend_tac(TAC *start)
{
    TAC *endTac = (TAC*)malloc(sizeof(TAC));
    endTac->op = tac_endproc;
    endTac->args.endproc.start = start;
    return endTac;
}

/**
 * Creates a new TAC block instruction.
 *
 * @param nvars : The number of new locals in the block.
 * @param vars : A list of the new locals in the block.
 * @return : The new TAC block instruction.
 */
TAC *new_block_tac(int nvars, TOKEN **vars)
{
    TAC *blockTac = (TAC*)malloc(sizeof(TAC));
    blockTac->op = tac_block;
    blockTac->args.block.nvars = nvars;
    blockTac->args.block.vars = vars;
    return blockTac;
}

/**
 * Creates a new TAC block end instruction.
 *
 * @return : The new TAC block end instruction.
 */
TAC *new_blockend_tac()
{
    TAC *endTac = (TAC*)malloc(sizeof(TAC));
    endTac->op = tac_endblock;
    return endTac;
}

/**
 * Creates a new TAC call instruction.
 *
 * @param name : The token name of the procedure to call.
 * @param arity : The number of arguments for the function call.
 * @return : The new TAC call instruction.
 */
TAC *new_call_tac(TOKEN *name, int arity)
{
    TAC *callTac = (TAC*)malloc(sizeof(TAC));
    callTac->op = tac_call;
    callTac->args.call.name = name;
    callTac->args.call.ar = (AR*)malloc(sizeof(AR));
    callTac->args.call.ar->arity = arity;
    return callTac;
}

/**
 * Creates a new TAC line instruction. These are standard straight line code
 * operations like addition, subtraction and multiplication.
 *
 * @param op : The TAC operation being created.
 * @param src1 : The first operand.
 * @param src2 : The second operand.
 * @param dst : The destination.
 * @return : The new TAC line instruction.
 */
TAC* new_line_tac(int op, TOKEN* src1, TOKEN* src2, TOKEN* dst)
{
    TAC* ans = (TAC*)malloc(sizeof(TAC));
    if (ans==NULL) {
        printf("Error! memory not allocated.");
        exit(0);
    }
    ans->op = op;
    ans->args.line.src1 = src1;
    ans->args.line.src2 = src2;
    ans->args.line.dst = dst;
    return ans;
}

/**
 * Creates a new temporary token. The format is t1, t2, etc. and then next
 * number is always taken from the current reg_count variable.
 *
 * @return : The new temporary token.
 */
TOKEN *new_temp() {
    TOKEN *t = new_token(TEMPORARY);
    char *tempName = (char*)malloc(10 * sizeof(char));
    sprintf(tempName, "t%d", reg_count);
    t->lexeme = tempName;
    reg_count++;
    return t;
}

/**
 * Prepends a TAC instruction to the start of a TAC sequence. TAC instructions
 * are stored in reverse order so we need to navigate to the end of the
 * sequence to prepend the TAC instruction.
 *
 * @param pre : The TAC instruction to prepend.
 * @param post : The TAC sequence to prepend the instruction to.
 * @return : The resulting TAC sequence.
 */
TAC *prepend_tac(TAC *pre, TAC *post) {
    TAC *n = post;
    while (n->next != NULL) n = n->next;
    n->next = pre;
    return post;
}

/**
 * Prints a sequence of TAC instructions to the command line.
 *
 * @param i : The TAC sequence to print.
 * @param ind : How many spaces to indent the TAC.
 */
void mmc_print_ic(TAC* i, int ind)
{
    if (i->next != NULL) mmc_print_ic(i->next, ind);
    for (int j = 0; j < ind; j++) {
        printf(" ");
    }
    switch(i->op) {
        case tac_load_const:
            printf("%s %s, %d\n",
                   tac_ops[i->op],
                   i->args.line.dst->lexeme,
                   i->args.line.src1->value);
            break;
        case tac_load_id:
            printf("%s %s, %s\n",
                   tac_ops[i->op],
                   i->args.line.dst->lexeme,
                   i->args.line.src1->lexeme);
            break;
        case tac_store:
            printf("%s %s, %s\n",
                   tac_ops[i->op],
                   i->args.line.src1->lexeme,
                   i->args.line.dst->lexeme);
            break;
        case tac_return:
            if (i->args.line.src1->type == CONSTANT)
                printf("%s %d\n",
                       tac_ops[i->op],
                       i->args.line.src1->value);
            else
                printf("%s %s\n",
                       tac_ops[i->op],
                       i->args.line.src1->lexeme);
            break;
        case tac_proc:
            printf("%s %s, %d, %d\n",
                   tac_ops[i->op],
                   i->args.proc.name->lexeme,
                   i->args.proc.ar->arity,
                   i->args.proc.ar->localCount);
            break;
        case tac_endproc:
            printf("%s\n", tac_ops[i->op]);
            break;
        case tac_block:
            printf("%s %d\n",
                   tac_ops[i->op],
                   i->args.block.nvars);
            break;
        case tac_endblock:
            printf("%s\n", tac_ops[i->op]);
            break;
        case tac_call:
            printf("%s %s, %d\n",
                   tac_ops[i->op],
                   i->args.call.name->lexeme,
                   i->args.call.ar->arity);
            break;
        case tac_label:
            printf("%s %s\n",
                   tac_ops[i->op],
                   i->args.taclabel.name->lexeme);
            break;
        case tac_if:
            printf("%s %s %s\n",
                   tac_ops[i->op],
                   i->args.tacif.cond->args.line.dst->lexeme,
                   i->args.tacif.label->name->lexeme);
            break;
        case tac_goto:
            printf("%s %s\n",
                   tac_ops[i->op],
                   i->args.tacgoto.label->name->lexeme);
            break;
        default:
            printf("%s %s, %s, %s\n",
                   tac_ops[i->op],
                   i->args.line.src1->lexeme,
                   i->args.line.src2->lexeme,
                   i->args.line.dst->lexeme);
    }
}
