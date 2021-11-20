#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "tac.h"
#include "token.h"
#include "C.tab.h"

int reg_count = 0;
int label_count = 1;

char* tac_ops[] = {"NOOP","LOAD","LOAD","STORE","MOD","MULTIPLY","ADD","SUBTRACT","DIVIDE","RETURN","PROC","BLOCK","CALL","LABEL","IF","GOTO","EQ","NE","GTEQ","GT","LTEQ","LT","BREAK","CONTINUE"};

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

TAC *new_if_tac(TAC *cond, TACLABEL *label)
{
    TAC *ifTac = (TAC*)malloc(sizeof(TAC));
    ifTac->op = tac_if;
    ifTac->args.tacif.cond = cond;
    ifTac->args.tacif.label = label;
    return ifTac;
}

TAC *new_goto_tac(TACLABEL *label)
{
    TAC *gotoTac = (TAC*)malloc(sizeof(TAC));
    gotoTac->op = tac_goto;
    gotoTac->args.tacgoto.label = label;
    return gotoTac;
}

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

TOKEN *new_temp() {
    TOKEN *t = new_token(TEMPORARY);
    char *tempName = (char*)malloc(10 * sizeof(char));
    sprintf(tempName, "t%d", reg_count);
    t->lexeme = tempName;
    reg_count++;
    return t;
}

TAC *prepend_tac(TAC *pre, TAC *post) {
    TAC *n = post;
    while (n->next != NULL) n = n->next;
    n->next = pre;
    return post;
}

void mmc_print_ic(TAC* i)
{
    if (i->next != NULL) mmc_print_ic(i->next);
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
                   tac_ops[i->op], // need to range check!
                   i->args.line.src1->lexeme,
                   i->args.line.src2->lexeme,
                   i->args.line.dst->lexeme);
    }
}

//QUEUE *prepend_queue(TAC* t, QUEUE* q) {
//    t->next = q->start;
//    q->start = t;
//}
//
//QUEUE *append_queue(TAC* t, QUEUE* q) {
//    q->end->next = t;
//    q->end = t;
//}

//TAC* make_tac(char *line, TAC *next)
//{
//    TAC *a = (TAC*)malloc(sizeof(TAC));
//    if (a==NULL) {
//        perror("Cannot make tac");
//        exit(1);
//    }
//    char buf[reg_count/10];
//    a->id = (char*)calloc(2 + (reg_count / 10), sizeof(char));
//    strcat(a->id, "L");
//    itoa(reg_count++, buf, 10);
//    strcat(a->id, buf);
//    a->line = line;
//    a->next = next;
//    return a;
//}
