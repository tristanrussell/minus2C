#ifndef __VALUE_H
#define __VALUE_H

#include "structs.h"

enum valuetype
{
    mmcNULL = 0,
    mmcINT = 1,
    mmcBOOL = 2,
    mmcSTRING = 3,
    mmcCLOSURE = 4,
    mmcRETURN = 5
};

VALUE *new_null(int);

VALUE* new_return(VALUE*);

VALUE *new_int(int);

VALUE *new_closure(FRAME*, NODE*);

void mmc_print(VALUE*);

#endif