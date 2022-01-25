#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include "nodes.h"
#include "env.h"
#include "tac.h"
#include "mc.h"
#include "C.tab.h"
#include "interpreter.h"
#include "icg.h"
#include "mcg.h"
#include "value.h"

char *named(int t)
{
    static char b[100];
    if (isgraph(t) || t==' ') {
      sprintf(b, "%c", t);
      return b;
    }
    switch (t) {
      default: return "???";
    case IDENTIFIER:
      return "id";
    case CONSTANT:
      return "constant";
    case STRING_LITERAL:
      return "string";
    case LE_OP:
      return "<=";
    case GE_OP:
      return ">=";
    case EQ_OP:
      return "==";
    case NE_OP:
      return "!=";
    case EXTERN:
      return "extern";
    case AUTO:
      return "auto";
    case INT:
      return "int";
    case VOID:
      return "void";
    case APPLY:
      return "apply";
    case LEAF:
      return "leaf";
    case IF:
      return "if";
    case ELSE:
      return "else";
    case WHILE:
      return "while";
    case CONTINUE:
      return "continue";
    case BREAK:
      return "break";
    case RETURN:
      return "return";
    }
}

void print_leaf(NODE *tree, int level)
{
    TOKEN *t = (TOKEN *)tree;
    int i;
    for (i=0; i<level; i++) putchar(' ');
    if (t->type == CONSTANT) printf("%d\n", t->value);
    else if (t->type == STRING_LITERAL) printf("\"%s\"\n", t->lexeme);
    else if (t) puts(t->lexeme);
}

void print_tree0(NODE *tree, int level)
{
    int i;
    if (tree==NULL) return;
    if (tree->type==LEAF) {
      print_leaf(tree->left, level);
    }
    else {
      for(i=0; i<level; i++) putchar(' ');
      printf("%s\n", named(tree->type));
/*       if (tree->type=='~') { */
/*         for(i=0; i<level+2; i++) putchar(' '); */
/*         printf("%p\n", tree->left); */
/*       } */
/*       else */
        print_tree0(tree->left, level+2);
      print_tree0(tree->right, level+2);
    }
}

void print_tree(NODE *tree)
{
    print_tree0(tree, 0);
}

extern int yydebug;
extern NODE* yyparse(void);
extern NODE* ans;
extern void init_symbtable(void);

int main(int argc, char** argv)
{
    int print = 0;
    int interpreter = 0;
    int tac = 0;
    int machine = 0;
    char *fileName = NULL;
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-p") == 0) {
            print = 1;
        } else if (strcmp(argv[i], "-i") == 0) {
            interpreter = 1;
        } else if (strcmp(argv[i], "-t") == 0) {
            tac = 1;
        } else if (strcmp(argv[i], "-c") == 0) {
            machine = 1;
        } else if (strcmp(argv[i], "-o") == 0) {
            if (i + 1 < argc && argv[i+1][0] != '-') {
                fileName = argv[i+1];
            }
        }
    }
    if (!interpreter && !tac && !machine) machine = 1;

    NODE* tree;
    if (argc>1 && strcmp(argv[1],"-d")==0) yydebug = 1;
    init_symbtable();
    printf("--C COMPILER\n");
    yyparse();
    tree = ans;
    printf("parse finished with %p\n\n", tree);
    if (print) print_tree(tree);

    if (interpreter) {
        FRAME *frame = new_frame();
        VALUE *val = interpret(tree, frame);

        // Print out the exit code, used in debugging.
//        if (val != NULL && val->type == mmcRETURN)
//            printf("\nProcess finished with exit code %d\n\n", val->v.ret->v.integer);
    }

    if (tac || machine) {
        TAC *tacSeq = generate_tac(tree);
        tacSeq = tac_optimise(tacSeq);
        if (tac) mmc_print_ic(tacSeq, 0);
        if (machine) {
            BB *bbSeq = bb_create(tacSeq);
            MC *mcSeq = mmc_mcg_bb(bbSeq);
            mmc_print_mc(mcSeq);
            mmc_print_file(mcSeq, fileName);
        }
    }
    return 0;
}
