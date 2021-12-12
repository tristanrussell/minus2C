#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include "nodes.h"
#include "token.h"
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

NODE *calc_leaf(NODE *tree)
{
	TOKEN *t = (TOKEN *)tree;
    NODE *leftLeaf;
    NODE *rightLeaf;

	switch (tree->type) {
    default: printf("???\n"); return NULL;
    case 37:
        printf("%%\n");
        leftLeaf = calc_leaf(tree->left);
        rightLeaf = calc_leaf(tree->right);

        if(leftLeaf->type == CONSTANT && rightLeaf->type == CONSTANT) {
            TOKEN *leftTok = (TOKEN*) leftLeaf;
            TOKEN *rightTok = (TOKEN*) rightLeaf;

            TOKEN *ans = new_token(CONSTANT);
            ans->value = leftTok->value % rightTok->value;

            return (NODE*) ans;
        } else {
            printf("err");
            return (NODE*) new_token(VOID);
        }
    case 42:
        printf("*\n");
        leftLeaf = calc_leaf(tree->left);
        rightLeaf = calc_leaf(tree->right);

        if(leftLeaf->type == CONSTANT && rightLeaf->type == CONSTANT) {
            TOKEN *leftTok = (TOKEN*) leftLeaf;
            TOKEN *rightTok = (TOKEN*) rightLeaf;

            TOKEN *ans = new_token(CONSTANT);
            ans->value = leftTok->value * rightTok->value;

            return (NODE*) ans;
        } else {
            printf("err");
            return (NODE*) new_token(VOID);
        }
    case 43:
        printf("+\n");
        leftLeaf = calc_leaf(tree->left);
        rightLeaf = calc_leaf(tree->right);

        if(leftLeaf->type == CONSTANT && rightLeaf->type == CONSTANT) {
            TOKEN *leftTok = (TOKEN*) leftLeaf;
            TOKEN *rightTok = (TOKEN*) rightLeaf;

            TOKEN *ans = new_token(CONSTANT);
            ans->value = leftTok->value + rightTok->value;

            return (NODE*) ans;
        } else {
            return (NODE*) new_token(VOID);
        }
    case 45:
        printf("-\n");
        leftLeaf = calc_leaf(tree->left);
        rightLeaf = calc_leaf(tree->right);

        if(leftLeaf->type == CONSTANT && rightLeaf->type == CONSTANT) {
            TOKEN *leftTok = (TOKEN*) leftLeaf;
            TOKEN *rightTok = (TOKEN*) rightLeaf;

            TOKEN *ans = new_token(CONSTANT);
            ans->value = leftTok->value - rightTok->value;

            return (NODE*) ans;
        } else {
            return (NODE*) new_token(VOID);
        }
    case 47:
        printf("/\n");
        leftLeaf = calc_leaf(tree->left);
        rightLeaf = calc_leaf(tree->right);

        if(leftLeaf->type == CONSTANT && rightLeaf->type == CONSTANT) {
            TOKEN *leftTok = (TOKEN*) leftLeaf;
            TOKEN *rightTok = (TOKEN*) rightLeaf;

            TOKEN *ans = new_token(CONSTANT);
            ans->value = leftTok->value / rightTok->value;

            return (NODE*) ans;
        } else {
            return (NODE*) new_token(VOID);
        }
    case 59:
        printf(";\n");
        return NULL;
    case 60:
        printf("<\n");
        leftLeaf = calc_leaf(tree->left);
        rightLeaf = calc_leaf(tree->right);

        if(leftLeaf->type == CONSTANT && rightLeaf->type == CONSTANT) {
            TOKEN *leftTok = (TOKEN*) leftLeaf;
            TOKEN *rightTok = (TOKEN*) rightLeaf;

            t->value = leftTok->value < rightTok->value;

            return (NODE*) tree;
        } else {
            return (NODE*) new_token(VOID);
        }
    case 61:
        printf("=\n");
        return NULL;
    case 62:
        printf(">\n");
        leftLeaf = calc_leaf(tree->left);
        rightLeaf = calc_leaf(tree->right);

        if(leftLeaf->type == CONSTANT && rightLeaf->type == CONSTANT) {
            TOKEN *leftTok = (TOKEN*) leftLeaf;
            TOKEN *rightTok = (TOKEN*) rightLeaf;

            t->value = leftTok->value > rightTok->value;

            return (NODE*) tree;
        } else {
            return (NODE*) new_token(VOID);
        }
    case 68:
        printf("D\n");
        return calc_leaf(tree->right);
    case 70:
        printf("F\n");
        return NULL;
    case 100:
        printf("d\n");
        return NULL;
    case 126:
        printf("~\n");
        return NULL;
    case IDENTIFIER:
        printf("id\n");
        return NULL;
    case CONSTANT:
        printf("const\n");
        return tree;
    case STRING_LITERAL:
        printf("string\n");
        return tree;
    case LE_OP:
        printf("<=\n");
        leftLeaf = calc_leaf(tree->left);
        rightLeaf = calc_leaf(tree->right);

        if(leftLeaf->type == CONSTANT && rightLeaf->type == CONSTANT) {
            TOKEN *leftTok = (TOKEN*) leftLeaf;
            TOKEN *rightTok = (TOKEN*) rightLeaf;

            t->value = leftTok->value <= rightTok->value;

            return (NODE*) tree;
        } else {
            return (NODE*) new_token(VOID);
        }
    case GE_OP:
        printf(">=\n");
        leftLeaf = calc_leaf(tree->left);
        rightLeaf = calc_leaf(tree->right);

        if(leftLeaf->type == CONSTANT && rightLeaf->type == CONSTANT) {
            TOKEN *leftTok = (TOKEN*) leftLeaf;
            TOKEN *rightTok = (TOKEN*) rightLeaf;

            t->value = leftTok->value >= rightTok->value;

            return (NODE*) tree;
        } else {
            return (NODE*) new_token(VOID);
        }
    case EQ_OP:
        printf("==\n");
        leftLeaf = calc_leaf(tree->left);
        rightLeaf = calc_leaf(tree->right);

        if(leftLeaf->type == CONSTANT && rightLeaf->type == CONSTANT) {
            TOKEN *leftTok = (TOKEN*) leftLeaf;
            TOKEN *rightTok = (TOKEN*) rightLeaf;

            t->value = leftTok->value == rightTok->value;

            return (NODE*) tree;
        } else {
            return (NODE*) new_token(VOID);
        }
    case NE_OP:
        printf("!=\n");
        leftLeaf = calc_leaf(tree->left);
        rightLeaf = calc_leaf(tree->right);

        if(leftLeaf->type == CONSTANT && rightLeaf->type == CONSTANT) {
            TOKEN *leftTok = (TOKEN*) leftLeaf;
            TOKEN *rightTok = (TOKEN*) rightLeaf;

            t->value = leftTok->value != rightTok->value;

            return (NODE*) tree;
        } else {
            return (NODE*) new_token(VOID);
        }
    case EXTERN:
        printf("extern\n");
        return NULL;
    case AUTO:
        printf("auto\n");
        return NULL;
    case INT:
        printf("int\n");
        return NULL;
    case VOID:
        printf("void\n");
        return NULL;
    case APPLY:
        printf("apply\n");
        return NULL;
    case LEAF:
        printf("leaf\n");
        return calc_leaf(tree->left);
    case IF:
        printf("if\n");
        leftLeaf = calc_leaf(tree->left);

        if (leftLeaf->type == VOID) {
            return leftLeaf;
        } else {
            TOKEN *leftTok = (TOKEN*) leftLeaf;
            if(leftTok->value) {
                return calc_leaf(tree->right->left);
            } else {
                return tree->right->type == ELSE ? calc_leaf(tree->right->right) : NULL;
            }
        }
    case ELSE:
        printf("else\n");
        return NULL;
    case WHILE:
        printf("while\n");
        return NULL;
    case CONTINUE:
        printf("continue\n");
        return NULL;
    case BREAK:
        printf("break\n");
        return NULL;
    case RETURN:
        printf("return\n");
        return calc_leaf(tree->left);
    }
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
        }
        if (strcmp(argv[i], "-i") == 0) {
            interpreter = 1;
        }
        if (strcmp(argv[i], "-t") == 0) {
            tac = 1;
        }
        if (strcmp(argv[i], "-c") == 0) {
            machine = 1;
        }
        if (strcmp(argv[i], "-o") == 0) {
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
    printf("parse finished with %p\n", tree);
    if (print) print_tree(tree);

    if (interpreter) {
        FRAME *frame = new_frame();
        VALUE *val = interpret(tree, frame);
        if (val != NULL && val->type == mmcRETURN) printf("\n%d\n\n", val->v.ret->v.integer);
    }

    if (tac || machine) {
        TAC *tacSeq = generate_tac(tree);
        tacSeq = tac_optimise(tacSeq);
        BB *bbSeq = bb_create(tacSeq);
        if (tac) bb_print(bbSeq);
        if (machine) {
            MC *mcSeq = mmc_mcg_bb(bbSeq);
            mmc_print_mc(mcSeq);
            mmc_print_file(mcSeq, fileName);
        }
    }
    return 0;
}
