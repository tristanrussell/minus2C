#ifndef __VALUE_H
#define __VALUE_H

#include "structs.h"

enum valuetype
{
    mmcINT = 1,
    mmcBOOL = 2,
    mmcSTRING = 3,
    mmcCLOSURE = 4
};

VALUE *new_int(int);

VALUE *new_closure(FRAME*, NODE*);

void mmc_print(VALUE*);

#endif