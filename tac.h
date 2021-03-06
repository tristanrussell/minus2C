#include "structs.h"

#define TEMPORARY 300
#define LABEL 301

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
    tac_endproc = 11,
    tac_block = 12,
    tac_endblock = 13,
    tac_call = 14,
    tac_label = 15,
    tac_if = 16,
    tac_goto = 17,
    tac_eq = 18,
    tac_ne = 19,
    tac_ge = 20,
    tac_gt = 21,
    tac_le = 22,
    tac_lt = 23,
    tac_break = 24,
    tac_continue = 25
};

TAC *new_label_tac();

TAC *new_if_tac(TAC*, TACLABEL*);

TAC *new_goto_tac(TACLABEL*);

TAC *new_proc_tac(TOKEN*, int);

TAC *new_procend_tac(TAC*);

TAC *new_block_tac(int, TOKEN**);

TAC *new_blockend_tac();

TAC *new_call_tac(TOKEN*, int);

TAC *new_line_tac(int, TOKEN*, TOKEN*, TOKEN*);

TOKEN *new_temp();

TAC *prepend_tac(TAC*, TAC*);

void mmc_print_ic(TAC*, int);
