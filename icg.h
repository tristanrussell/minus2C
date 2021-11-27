#ifndef MINUS2C_ICG_H
#define MINUS2C_ICG_H

#include "structs.h"

TAC* mmc_icg(NODE*);

void remove_blocks(TAC *tac);

TAC* tac_optimise(TAC*);

BB* bb_create(TAC*);

BB* bb_optimise(BB*);

void bb_print(BB*);

#endif //MINUS2C_ICG_H
