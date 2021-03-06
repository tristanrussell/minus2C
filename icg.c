#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "icg.h"
#include "tac.h"
#include "C.tab.h"
#include "token.h"
#include "llist.h"

// The global activation record.
static AR *globalAR = NULL;

/**
 * Converts a list of tokens into an array of tokens.
 *
 * @param list : The linked list of tokens.
 * @return : The array of tokens.
 */
TOKEN **convert_token_array(LLIST *list)
{
    TOKEN **tokens = (TOKEN**)malloc(count_list(list) * sizeof(TOKEN*));
    LLIST *curr = list;
    int i = 0;
    while (curr != NULL) {
        tokens[i++] = (TOKEN*)curr->item;
        curr = curr->next;
    }
    return tokens;
}

/**
 * Takes the root node of the start of a list of parameters for a function
 * declaration and returns the parameters as a list.
 *
 * @param ast : The root node of the parameters.
 * @return : The linked list of parameters.
 */
LLIST *getParams(NODE *ast)
{
    switch (ast->type) {
        case ',':
            return join_llist(getParams(ast->left), getParams(ast->right));
        case '~':
            return getParams(ast->right);
        case LEAF:
            return new_llist((void*)ast->left);
        default:
            return NULL;
    }
}

/**
 * A recursive function used for calculating the list of locals in a function
 * body.
 *
 * @param ast : The node to be traversed to find the locals.
 * @return : The linked list of locals.
 */
LLIST *computeLocalsChain(NODE *ast)
{
    switch (ast->type) {
        case ',':
            return join_llist(computeLocalsChain(ast->left), computeLocalsChain(ast->right));
        case '=':
            return computeLocalsChain(ast->left);
        case LEAF:
            return new_llist((void*)ast->left);
        default:
            return NULL;
    }
}

/**
 * Takes the root node of the start of a procedure body and returns a list of
 * locals declared in the code.
 *
 * @param ast : The root node of the function body.
 * @return : The linked list of locals.
 */
LLIST *getLocals(NODE *ast)
{
    LLIST *left;
    LLIST *right;

    switch (ast->type) {
        case ';':
            left = getLocals(ast->left);
            right = getLocals(ast->right);
            if (left == NULL && right == NULL) return NULL;
            if (left == NULL) return right;
            if (right == NULL) return left;
            return join_llist(left, right);
        case ',':
            return join_llist(getLocals(ast->left), getLocals(ast->right));
        case '~':
            return computeLocalsChain(ast->right);
        case 'D':
            return new_llist((void*)ast->left->right->left->left);
        case LEAF:
            return new_llist((void*)ast->left);
        default:
            return NULL;
    }
}

/**
 * Takes the root node of the start of a list of arguments for a function call
 * and returns the arguments as a list.
 *
 * @param ast : The root node of the arguments.
 * @return : The linked list of arguments.
 */
LLIST *getArgs(NODE *ast)
{
    switch(ast->type) {
        case ',':
            return join_llist(getArgs(ast->left), getArgs(ast->right));
        case APPLY:
        case LEAF:
            return new_llist((void*)ast->left);
        default:
            return NULL;
    }
}

/**
 * Takes the root node of the AST and returns a list of globals.
 *
 * @param ast : The root node of the AST.
 * @return : The linked list of globals.
 */
LLIST *getGlobals(NODE *ast)
{
    LLIST *left;
    LLIST *right;

    switch (ast->type) {
        case ';':
        case '~':
            left = getGlobals(ast->left);
            right = getGlobals(ast->right);
            if (left == NULL && right == NULL) return NULL;
            if (left == NULL) return right;
            if (right == NULL) return left;
            return join_llist(left, right);
        case '=':
            return getGlobals(ast->left);
        case ',':
            return join_llist(getGlobals(ast->left), getGlobals(ast->right));
        case 'D':
            return new_llist((void*)ast->left->right->left->left);
        case LEAF:
            if (ast->left->type == IDENTIFIER) return new_llist((void*)ast->left);
            else return NULL;
        default:
            return NULL;
    }
}

/**
 * Handles the compilation of an if statement into TAC.
 *
 * @param ast : The IF node indicating the start of the if statement.
 * @return : The compiled TAC sequence.
 */
TAC *tac_compute_if(NODE *ast)
{
    TOKEN *temp;

    TAC *leftLeaf = mmc_icg(ast->left);
    switch (leftLeaf->op) {
        case tac_eq:
            leftLeaf->op = tac_ne;
            break;
        case tac_ne:
            leftLeaf->op = tac_eq;
            break;
        case tac_gt:
            leftLeaf->op = tac_le;
            break;
        case tac_ge:
            leftLeaf->op = tac_lt;
            break;
        case tac_lt:
            leftLeaf->op = tac_ge;
            break;
        case tac_le:
            leftLeaf->op = tac_gt;
            break;
        default:
            temp = new_token(CONSTANT);
            temp->value = 0;
            TAC *load = new_line_tac(tac_load_const, temp, NULL, new_temp());
            TAC *comp = new_line_tac(tac_eq, leftLeaf->args.line.dst, load->args.line.dst, new_temp());
            comp->next = load;
            load->next = leftLeaf;
            leftLeaf = comp;
    }

    if (ast->right->type == ELSE) {
        TAC *thenLeaf = mmc_icg(ast->right->left);
        LLIST *localsThen = getLocals(ast->right->left);
        int countThen = count_list(localsThen);

        TAC *blockThenStart = new_block_tac(countThen, convert_token_array(localsThen));
        TAC *blockThenEnd = new_blockend_tac();

        TAC *elseLeaf = mmc_icg(ast->right->right);
        LLIST *localsElse = getLocals(ast->right->right);
        int countElse = count_list(localsElse);

        TAC *blockElseStart = new_block_tac(countElse, convert_token_array(localsElse));
        TAC *blockElseEnd = new_blockend_tac();

        TAC *labelElse = new_label_tac();
        TAC *labelEnd = new_label_tac();

        TAC *ifTac = new_if_tac(leftLeaf, &labelElse->args.taclabel);
        ifTac->next = leftLeaf;
        blockThenStart->next = ifTac;
        prepend_tac(blockThenStart, thenLeaf);

        TAC *gotoEnd = new_goto_tac(&labelEnd->args.taclabel);
        gotoEnd->next = thenLeaf;
        blockThenEnd->next = gotoEnd;

        labelElse->next = blockThenEnd;
        blockElseStart->next = labelElse;

        prepend_tac(blockElseStart, elseLeaf);
        blockElseEnd->next = elseLeaf;

        labelEnd->next = blockElseEnd;
        return labelEnd;
    } else {
        TAC *rightLeaf = mmc_icg(ast->right);
        LLIST *locals = getLocals(ast->right);
        int count = count_list(locals);

        TAC *blockStart = new_block_tac(count, convert_token_array(locals));
        TAC *blockEnd = new_blockend_tac();

        TAC *label = new_label_tac();

        TAC *ifTac = new_if_tac(leftLeaf, &label->args.taclabel);
        ifTac->next = leftLeaf;
        blockStart->next = ifTac;
        prepend_tac(blockStart, rightLeaf);

        blockEnd->next = rightLeaf;
        label->next = blockEnd;
        return label;
    }
}

/**
 * Handles the compilation of a while loop into TAC.
 *
 * @param ast : The WHILE node indicating the start of the while loop.
 * @return : The compiled TAC sequence.
 */
TAC *tac_compute_while(NODE *ast)
{
    TOKEN *temp;

    TAC *leftLeaf = mmc_icg(ast->left);
    switch (leftLeaf->op) {
        case tac_eq:
            leftLeaf->op = tac_ne;
            break;
        case tac_ne:
            leftLeaf->op = tac_eq;
            break;
        case tac_gt:
            leftLeaf->op = tac_le;
            break;
        case tac_ge:
            leftLeaf->op = tac_lt;
            break;
        case tac_lt:
            leftLeaf->op = tac_ge;
            break;
        case tac_le:
            leftLeaf->op = tac_gt;
            break;
        default:
            temp = new_token(CONSTANT);
            temp->value = 0;
            TAC *load = new_line_tac(tac_load_const, temp, NULL, new_temp());
            TAC *comp = new_line_tac(tac_eq, leftLeaf->args.line.dst, load->args.line.dst, new_temp());
            comp->next = load;
            load->next = leftLeaf;
            leftLeaf = comp;
    }

    TAC *labelStart = new_label_tac();
    TAC *rightLeaf = mmc_icg(ast->right);
    TAC *labelEnd = new_label_tac();
    TAC *ifTac = new_if_tac(leftLeaf, &labelEnd->args.taclabel);
    TAC *gotoTac = new_goto_tac(&labelStart->args.taclabel);

    LLIST *locals = getLocals(ast->right);
    int count = count_list(locals);
    TAC *blockStart = new_block_tac(count, convert_token_array(locals));
    TAC *blockEnd = new_blockend_tac();

    TAC *r = rightLeaf;
    while (r != NULL) {
        if (r->op == tac_continue) {
            r->op = tac_goto;
            r->args.tacgoto.label = &labelStart->args.taclabel;
        } else if (r->op == tac_break) {
            r->op = tac_goto;
            r->args.tacgoto.label = &labelEnd->args.taclabel;
        }

        r = r->next;
    }

    prepend_tac(labelStart, leftLeaf);
    ifTac->next = leftLeaf;
    blockStart->next = ifTac;
    prepend_tac(blockStart, rightLeaf);

    gotoTac->next = rightLeaf;
    blockEnd->next = gotoTac;
    labelEnd->next = blockEnd;
    return labelEnd;
}

/**
 * Handles the compilation of a function declaration into TAC.
 *
 * @param ast : The D node for the function declaration.
 * @return : The compiled TAC sequence.
 */
TAC *tac_compute_closure(NODE *ast)
{
    if (ast->type != 'D') return NULL;

    NODE *dec = ast->left;

    LLIST *locals = getLocals(ast->right);
    int count = count_list(locals);
    TAC *blockStart = new_block_tac(count, convert_token_array(locals));
    TAC *blockEnd = new_blockend_tac();

    TAC *code = mmc_icg(ast->right);

    if (dec->type != 'd' || code == NULL) {
        printf("Error in function declaration.\n");
        exit(EXIT_FAILURE);
    }

    NODE *func = dec->right;

    if (func->type != 'F') {
        printf("Error in function declaration.\n");
        exit(EXIT_FAILURE);
    }

    TAC *name = mmc_icg(func->left);

    LLIST *params = getParams(func->right);
    int numParams = count_list(params);

    TAC *proc = new_proc_tac(name->args.line.src1, numParams);
    proc->args.proc.ar->param = convert_token_array(params);
    TAC *endproc = new_procend_tac(proc);

    blockStart->next = proc;

    prepend_tac(blockStart, code);

    blockEnd->next = code;
    endproc->next = blockEnd;

    return endproc;
}

/**
 * A structure used for storing a linked list of TAC sequences.
 */
typedef struct tlist {
    TAC *tac;
    struct tlist *next;
} TLIST;

/**
 * An early attempt at enabling function calls and arithmetic operations as
 * arguments.
 * - Needs further work to get it working.
 *
 * @param ast
 * @return
 */
TLIST *tac_compute_args(NODE *ast)
{
    TLIST *left;
    TLIST *right;

    switch(ast->type) {
        case ',':
            left = tac_compute_args(ast->left);
            right = tac_compute_args(ast->right);
            right->next = left;
            return right;
        case APPLY:
        case LEAF:
            left = (TLIST *)malloc(sizeof(TLIST));
            left->tac = mmc_icg(ast);
            return left;
        default:
            return NULL;
    }
}

/**
 * Handles the compilation of a function call into TAC.
 *
 * @param ast : The APPLY node for the function call.
 * @return : The compiled TAC sequence.
 */
TAC *tac_compute_call(NODE *ast)
{
    TAC *leftLeaf = mmc_icg(ast->left);
    TAC *ret = new_call_tac(leftLeaf->args.line.src1, 0);

    int numArgs;
    LLIST *args;
    if (ast->right != NULL) {
        args = getArgs(ast->right);
        numArgs = count_list(args);
    } else {
        args = NULL;
        numArgs = 0;
    }

    /*
     * The following commented code is an early attempt at enabling function
     * calls and arithmetic operations as arguments.
     *
     * TLIST *list = tac_compute_args(ast->right);
     * TLIST *curr = list;
     * TLIST *prev = NULL;
     * while (curr != NULL) {
     *     countArgs++;
     *     if (curr->tac->op == tac_call) {
     *         prepend_tac(curr->tac, ret);
     *         if (prev != NULL) {
     *             prev->next = curr->next;
     *         } else {
     *             list = curr->next;
     *             continue;
     *         }
     *     }
     *     prev = curr;
     *     curr = curr->next;
     * }
     * curr = list;
     * while (curr != NULL) {
     *     prepend_tac(curr->tac, ret);
     *     curr = curr->next;
     * }
     */

    ret->args.call.ar->arity = numArgs;
    ret->args.call.ar->param = convert_token_array(args);
    return ret;
}

TAC *mmc_icg(NODE* ast)
{
    TOKEN *t = (TOKEN*)ast;
    TAC *leftLeaf = NULL;
    TAC *rightLeaf = NULL;
    TAC *ret = NULL;

    switch (ast->type) {
        case '%':
            leftLeaf = mmc_icg(ast->left);
            rightLeaf = mmc_icg(ast->right);
            ret = new_line_tac(tac_mod, leftLeaf->args.line.dst, rightLeaf->args.line.dst, new_temp());
            prepend_tac(leftLeaf, rightLeaf);
            ret->next = rightLeaf;
            return ret;
        case '*':
            leftLeaf = mmc_icg(ast->left);
            rightLeaf = mmc_icg(ast->right);
            ret = new_line_tac(tac_times, leftLeaf->args.line.dst, rightLeaf->args.line.dst, new_temp());
            prepend_tac(leftLeaf, rightLeaf);
            ret->next = rightLeaf;
            return ret;
        case '+':
            leftLeaf = mmc_icg(ast->left);
            rightLeaf = mmc_icg(ast->right);
            ret = new_line_tac(tac_plus, leftLeaf->args.line.dst, rightLeaf->args.line.dst, new_temp());
            prepend_tac(leftLeaf, rightLeaf);
            ret->next = rightLeaf;
            return ret;
        case '-':
            leftLeaf = mmc_icg(ast->left);
            rightLeaf = mmc_icg(ast->right);
            ret = new_line_tac(tac_sub, leftLeaf->args.line.dst, rightLeaf->args.line.dst, new_temp());
            prepend_tac(leftLeaf, rightLeaf);
            ret->next = rightLeaf;
            return ret;
        case '/':
            leftLeaf = mmc_icg(ast->left);
            rightLeaf = mmc_icg(ast->right);
            ret = new_line_tac(tac_div, leftLeaf->args.line.dst, rightLeaf->args.line.dst, new_temp());
            prepend_tac(leftLeaf, rightLeaf);
            ret->next = rightLeaf;
            return ret;
        case ';':
            leftLeaf = mmc_icg(ast->left);
            rightLeaf = mmc_icg(ast->right);
            return prepend_tac(leftLeaf, rightLeaf);
        case '<':
            leftLeaf = mmc_icg(ast->left);
            rightLeaf = mmc_icg(ast->right);
            prepend_tac(leftLeaf, rightLeaf);
            ret = new_line_tac(tac_lt, leftLeaf->args.line.dst, rightLeaf->args.line.dst, new_temp());
            ret->next = rightLeaf;
            return ret;
        case '=':
            leftLeaf = mmc_icg(ast->left);
            rightLeaf = mmc_icg(ast->right);
            if (rightLeaf->op == tac_call) {
                TOKEN *rv = new_token(TEMPORARY);
                rv->lexeme = "v0";
                ret = new_line_tac(tac_store, rv, NULL, leftLeaf->args.line.src1);
            } else {
                ret = new_line_tac(tac_store, rightLeaf->args.line.dst, NULL, leftLeaf->args.line.src1);
            }
            ret->next = rightLeaf;
            return ret;
        case '>':
            leftLeaf = mmc_icg(ast->left);
            rightLeaf = mmc_icg(ast->right);
            prepend_tac(leftLeaf, rightLeaf);
            ret = new_line_tac(tac_gt, leftLeaf->args.line.dst, rightLeaf->args.line.dst, new_temp());
            ret->next = rightLeaf;
            return ret;
        case 'D':
            return tac_compute_closure(ast);
        case '~':
        case ',':
            leftLeaf = mmc_icg(ast->left);
            rightLeaf = mmc_icg(ast->right);
            if (leftLeaf != NULL && rightLeaf != NULL) {
                prepend_tac(leftLeaf, rightLeaf);
                return rightLeaf;
            } else if (leftLeaf != NULL) return leftLeaf;
            else if (rightLeaf != NULL) return rightLeaf;
            else return NULL;
        case IDENTIFIER:
            return new_line_tac(tac_load_id, t, NULL, new_temp());
        case CONSTANT:
            return new_line_tac(tac_load_const, t, NULL, new_temp());
        case LE_OP:
            leftLeaf = mmc_icg(ast->left);
            rightLeaf = mmc_icg(ast->right);
            prepend_tac(leftLeaf, rightLeaf);
            ret = new_line_tac(tac_le, leftLeaf->args.line.dst, rightLeaf->args.line.dst, new_temp());
            ret->next = rightLeaf;
            return ret;
        case GE_OP:
            leftLeaf = mmc_icg(ast->left);
            rightLeaf = mmc_icg(ast->right);
            prepend_tac(leftLeaf, rightLeaf);
            ret = new_line_tac(tac_ge, leftLeaf->args.line.dst, rightLeaf->args.line.dst, new_temp());
            ret->next = rightLeaf;
            return ret;
        case EQ_OP:
            leftLeaf = mmc_icg(ast->left);
            rightLeaf = mmc_icg(ast->right);
            prepend_tac(leftLeaf, rightLeaf);
            ret = new_line_tac(tac_eq, leftLeaf->args.line.dst, rightLeaf->args.line.dst, new_temp());
            ret->next = rightLeaf;
            return ret;
        case NE_OP:
            leftLeaf = mmc_icg(ast->left);
            rightLeaf = mmc_icg(ast->right);
            prepend_tac(leftLeaf, rightLeaf);
            ret = new_line_tac(tac_ne, leftLeaf->args.line.dst, rightLeaf->args.line.dst, new_temp());
            ret->next = rightLeaf;
            return ret;
        case INT:
            return NULL;
        case VOID:
            return NULL;
        case FUNCTION:
            return NULL;
        case APPLY:
            return tac_compute_call(ast);
        case LEAF:
            return mmc_icg(ast->left);
        case IF:
            return tac_compute_if(ast);
        case ELSE:
            return NULL;
        case WHILE:
            return tac_compute_while(ast);
        case CONTINUE:
            return new_line_tac(tac_continue, NULL, NULL, NULL);
        case BREAK:
            return new_line_tac(tac_break, NULL, NULL, NULL);
        case RETURN:
            leftLeaf = mmc_icg(ast->left);
            if (leftLeaf->op == tac_load_id || leftLeaf->op == tac_load_const) {
                return new_line_tac(tac_return, leftLeaf->args.line.src1, NULL, NULL);
            } else if (leftLeaf->op == tac_call) {
                TOKEN *rv = new_token(TEMPORARY);
                rv->lexeme = "v0";
                ret = new_line_tac(tac_return, rv, NULL, NULL);
                ret->next = leftLeaf;
                return ret;
            } else {
                ret = new_line_tac(tac_return, leftLeaf->args.line.dst, NULL, NULL);
                ret->next = leftLeaf;
                return ret;
            }
        default:
            printf("unknown type code %d (%p) in mmc_icg\n",ast->type,ast);
            return NULL;
    }
}

/**
 * The entry function for generating the TAC sequence.
 *
 * @param tree : The AST tree to be compiled into TAC.
 * @return : The compiled TAC sequence.
 */
TAC *generate_tac(NODE *tree)
{
    globalAR = (AR*)malloc(sizeof(AR));
    LLIST *locals = getGlobals(tree);
    globalAR->local = convert_token_array(locals);
    globalAR->localCount = count_list(locals);
    return mmc_icg(tree);
}

/**
 * Removes TAC blocks and adds the locals declared in the blocks to the
 * enclosing procedures.
 *
 * @param tac : The TAC sequence with the TAC blocks.
 */
void remove_blocks(TAC *tac)
{
    int count = 0;

    TAC *curr = tac;
    LLIST *list = new_llist(NULL);

    while (curr->op != tac_endproc && curr->next != NULL) curr = curr->next;

    if (curr->next == NULL) return;

    TAC *prev = curr;
    curr = curr->next;
    for (; curr != NULL; prev = curr, curr = curr->next) {
        if (curr->op == tac_block) {
            count += curr->args.block.nvars;
            for (int i = 0; i < count; i++) {
                list = append_llist(list, curr->args.block.vars[i]);
            }
            prev->next = curr->next;
        } else if (curr->op == tac_endblock) {
            prev->next = curr->next;
        } else if (curr->op == tac_endproc) {
            remove_blocks(curr);
            curr = curr->args.endproc.start;
        } else if (curr->op == tac_proc) {
            curr->args.proc.ar->localCount = count;
            curr->args.proc.ar->local = convert_token_array(list->next);
            if (curr->next != NULL) remove_blocks(curr->next);
            return;
        }
    }
}

/**
 * Add static links to all the activation records.
 *
 * @param tac : The TAC sequence containing the activation records.
 */
void add_static_links(TAC *tac)
{
    TAC *curr = tac;
    AR *ar = globalAR;

    while (curr->op != tac_endproc && curr->next != NULL) curr = curr->next;

    if (curr->next == NULL) return;

    for (; curr != NULL; curr = curr->next) {
        if (curr->op == tac_endproc) {
            curr->args.endproc.start->args.proc.ar->sl = ar;
            ar = curr->args.endproc.start->args.proc.ar;
        } else if (curr->op == tac_proc) {
            ar = curr->args.proc.ar->sl;
        } else if (curr->op == tac_call) {
            curr->args.call.ar->fp = ar;
        }
    }
}

/**
 * A recursive sub function for flattening procedures.
 *
 * @param tac : The TAC sequence to perform procedure flattening on.
 * @return : The TAC sequence with flattened procedures.
 */
TAC *flatten_proc_rec(TAC *tac)
{
    TAC *ret = tac;
    TAC *end = tac->args.endproc.start;
    TAC *curr = tac->next;
    TAC *prev = tac;
    for (; curr != NULL; prev = curr, curr = curr->next) {
        if (curr == end) break;
        else if (curr->op == tac_endproc) {
            prev->next = curr->args.endproc.start->next;
            curr->args.endproc.start->next = NULL;
            ret = flatten_proc_rec(curr);
            prepend_tac(tac, ret);
            curr = prev;
        }
    }
    return ret;
}

/**
 * Flattens procedures so that there are no embedded procedure declarations.
 *
 * @param tac : The TAC sequence within which to flatten procedures.
 */
void flatten_procedures(TAC *tac)
{
    TAC *curr = tac;
    TAC *next = NULL;

    while(curr->op != tac_endproc && curr->next != NULL) curr = curr->next;

    if (curr->next == NULL) return;

    TAC *prev = NULL;
    for (; curr != NULL; prev = curr, curr = curr->next) {
        if (curr->op == tac_endproc) {
            next = curr->args.endproc.start->next;
            curr->args.endproc.start->next = NULL;
            curr = flatten_proc_rec(curr);
            if (prev != NULL) prev->next = curr;
            while (curr->next != NULL) curr = curr->next;
            curr->next = next;
        }
    }
}

/**
 * Removes TAC instructions after the main procedure.
 *
 * @param tac : The TAC sequence to remove the post main code from.
 * @return : The TAC sequence after post main code has been removed.
 */
TAC *remove_post_main(TAC *tac)
{
    TAC *curr = tac;
    while(curr != NULL && (curr->op != tac_endproc || strcmp(curr->args.endproc.start->args.proc.name->lexeme, "main") != 0)) curr = curr->next;
    return curr;
}

/**
 * Performs some optimisations on a sequence of TAC instructions.
 *
 * @param tac : The sequence of TAC instructions to optimise.
 * @return : The optimised sequence of TAC instructions.
 */
TAC *tac_optimise(TAC *tac)
{
    remove_blocks(tac);
    TAC *ret = remove_post_main(tac);
    add_static_links(ret);
    flatten_procedures(ret);
    return ret;
}

/**
 * Creates a sequence of basic blocks from a sequence of TAC instructions.
 *
 * @param seq : The sequence of TAC instructions to convert to basic blocks.
 * @return : The sequence of basic blocks.
 */
BB *bb_create(TAC* seq)
{
    BB *bb;
    if (seq->next == NULL) {
        bb = (BB*)malloc(sizeof(BB));
        return bb;
    } else bb = bb_create(seq->next);

    int jumpOps[] = {tac_proc, tac_label};
    int len = 4;
    for (int i = 0; i < len; i++) {
        if (seq->op == jumpOps[i]) {
            BB *nextBB = (BB*)malloc(sizeof(BB));
            nextBB->next = bb;
            seq->next = NULL;
            nextBB->leader = seq;
            return nextBB;
        }
    }

    bb->leader = seq;
    return bb;
}

/**
 * Performs intra-basic block optimisations.
 * - Not yet implemented, left for future improvements.
 *
 * @param bb : The basic block to optimise.
 * @return : The optimised basic block.
 */
BB* bb_optimise(BB* bb)
{
    return (BB*)0;
}

/**
 * Prints a sequence of basic blocks.
 *
 * @param bb : The sequence of basic blocks to print.
 */
void bb_print(BB* bb)
{
    if (bb->next != NULL) bb_print(bb->next);

    printf("BASIC BLOCK START\n");

    mmc_print_ic(bb->leader, 2);

    printf("BASIC BLOCK END\n");
}