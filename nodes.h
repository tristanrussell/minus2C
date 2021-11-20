#ifndef __NODES_H
#define __NODES_H

#include "structs.h"

NODE* make_leaf(TOKEN*);
NODE* make_node(int, NODE*, NODE*);

#endif