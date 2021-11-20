#ifndef MINUS2C_ICG_H
#define MINUS2C_ICG_H

#include "structs.h"

TAC* mmc_icg(NODE*);

BB* bb_create(TAC*);

BB* bb_optimise(BB*);

void bb_print(BB*);

#endif //MINUS2C_ICG_H
