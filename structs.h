#ifndef __STRUCTS_H
#define __STRUCTS_H

typedef struct frame FRAME;
typedef struct binding BINDING;
typedef struct closure CLOSURE;
typedef struct value VALUE;
typedef struct node NODE;
typedef struct token TOKEN;
typedef struct taclabel TACLABEL;
typedef struct tacif TACIF;
typedef struct tacgoto TACGOTO;
typedef struct tac TAC;
typedef struct bb BB;
typedef struct ar AR;
typedef struct mc MC;

typedef struct frame {
    BINDING*  bindings;
    FRAME*    next;
} FRAME;

typedef struct binding {
    TOKEN*    name;
    VALUE*    val;
    BINDING*  next;
} BINDING;

typedef struct closure
{
    FRAME*  env;
    NODE*   ids;
    int     type;
    NODE*   code;
} CLOSURE;

typedef struct value
{
    int  type;
    union {
        int       integer;
        int       boolean;
        char*     string;
        CLOSURE*  closure;
        VALUE*    ret;
    } v;
} VALUE;

typedef struct node {
    int    type;
    NODE*  left;
    NODE*  right;
} NODE;

typedef struct token {
    int     type;
    char*   lexeme;
    int     value;
    TOKEN*  next;
} TOKEN;

typedef struct proc {
    TOKEN*  name;
    AR*     ar;
} PROC;

typedef struct endproc {
    TAC*  start;
} ENDPROC;

typedef struct block {
    int      nvars;
    TOKEN**  vars;
} BLOCK;

typedef struct call {
    TOKEN*  name;
    AR*     ar;
} CALL;

typedef struct line {
    TOKEN*  src1;
    TOKEN*  src2;
    TOKEN*  dst;
} LINE;

typedef struct taclabel {
    TOKEN*  name;
} TACLABEL;

typedef struct tacif {
    TAC*       cond;
    TACLABEL*  label;
} TACIF;

typedef struct tacgoto {
    TACLABEL*  label;
} TACGOTO;

typedef struct tac {
    int  op;
    union {
        PROC      proc;
        ENDPROC   endproc;
        BLOCK     block;
        CALL      call;
        LINE      line;
        TACLABEL  taclabel;
        TACIF     tacif;
        TACGOTO   tacgoto;
    } args;
    TAC*  next;
} TAC;

typedef struct bb {
    TAC*  leader;
    BB*   next;
} BB;

typedef struct ar {
    AR*      fp;
    char     pc;
    AR*      sl;
    TOKEN**  param;
    int      arity;
    TOKEN**  local;
    int      localCount;
    TOKEN**  tmp;
    int      tmpCount;
} AR;

typedef struct mc {
    char*  insn;
    MC*    next;
} MC;

#endif //__STRUCTS_H
