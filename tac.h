#include "structs.h"

#define TEMPORARY 300
#define LABEL 301

//typedef struct tac_queue {
//    TAC*    start;
//    TAC*    end;
//} QUEUE;

enum tac_op
{
    tac_load_const = 1,
    tac_load_id = 2,
    tac_store = 3,
    tac_mod = 4,
    tac_times = 5,
    tac_plus = 6,
    tac_sub = 7,
    tac_div = 8,
    tac_return = 9,
    tac_proc = 10,
    tac_block = 11,
    tac_call = 12,
    tac_label = 13,
    tac_if = 14,
    tac_goto = 15,
    tac_eq = 16,
    tac_ne = 17,
    tac_ge = 18,
    tac_gt = 19,
    tac_le = 20,
    tac_lt = 21,
    tac_break = 22,
    tac_continue = 23
};

TAC *new_label_tac();

TAC *new_if_tac(TAC*, TACLABEL*);

TAC *new_goto_tac(TACLABEL*);

TAC *new_line_tac(int, TOKEN*, TOKEN*, TOKEN*);

TOKEN *new_temp();

TAC *prepend_tac(TAC*, TAC*);

void mmc_print_ic(TAC*);

//QUEUE *prepend_queue(TAC*, QUEUE*);
//
//QUEUE *append_queue(TAC*, QUEUE*);

//typedef struct call {
//    TOKEN* name;
//    int arity;
//} CALL;
//
//typedef struct block {
//    int nvars;
//} BLOCK;
//
//typedef struct tac {
//    int         op;
//    union {
//        BLOCK block;
//        CALL call;
//    } args;
//    TAC*        next;
//} TAC;
