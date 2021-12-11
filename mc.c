#include <stdlib.h>
#include <stdio.h>
#include "mc.h"

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

void mmc_print_mc(MC* i)
{
    for (MC *curr = i; curr != NULL; curr = curr->next) {
        printf("%s\n",curr->insn);
    }
}

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