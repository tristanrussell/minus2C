#ifndef __ENV_H
#define __ENV_H

#include "structs.h"

FRAME *new_frame();

FRAME *extend_frame(FRAME*, NODE*, NODE*);

VALUE *lookup_name(TOKEN*, FRAME*);

VALUE *assign_name(TOKEN*, FRAME*, VALUE*);

VALUE *declare_closure(TOKEN*, FRAME*, NODE*, NODE*, int);

VALUE *declare_name(TOKEN*, FRAME*, int);

#endif //__ENV_H