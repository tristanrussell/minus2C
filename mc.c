#include <stdlib.h>
#include <stdio.h>
#include "mc.h"

/**
 * Creates a new machine code structure with the given instruction.
 *
 * @param s : The machine code instruction.
 * @return : The new machine code instruction.
 */
MC* new_mci(char* s)
{
    MC* ans = (MC*)malloc(sizeof(MC));
    if (ans==NULL) {
        printf("Error! memory not allocated.");
        exit(0);
    }
    ans->insn = s;
    ans->next = NULL;
    return ans;
}

/**
 * Prints a list of machine code instructions to the terminal.
 *
 * @param i : The leading machine code instruction.
 */
void mmc_print_mc(MC* i)
{
    for (MC *curr = i; curr != NULL; curr = curr->next) {
        printf("%s\n",curr->insn);
    }
}

/**
 * Prints a list of machine code instructions to a file.
 *
 * @param i : The leading machine code instruction.
 * @param name : The name of the file to print to.
 */
void mmc_print_file(MC *i, char *name)
{
    char *fileName = name;

    if (fileName == NULL) {
        fileName = (char*)malloc(10 * sizeof(char));
        sprintf(fileName, "a.s");
    }

    FILE *f = fopen(fileName, "w");

    for (MC *curr = i; curr != NULL; curr = curr->next) {
        fprintf(f, "%s\n", curr->insn);
    }

    fclose(f);
}