#include "dump.h"

#include <execinfo.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <signal.h>
/* Obtain a backtrace and print it to @code{stdout}. */
void print_trace(FILE *fp)
{
    void *array[10];
    size_t size;
    char **strings;
    size_t i;

    size = backtrace(array, 10);
    strings = backtrace_symbols(array, size);
    if (NULL == strings)
    {
        fprintf(fp, "backtrace_synbols:%s\n", strerror(errno));
        return;
    }

    fprintf(fp,"Obtained %zd stack frames.\n", size);

    for (i = 0; i < size; i++)
        fprintf(fp,"%s\n", strings[i]);

    free(strings);
}

void dump_func(const char *output_path)
{
    FILE *fp = fopen(output_path, "a");
    if (fp == NULL)
    {
        print_trace(stderr);
    }
    else
    {
        print_trace(fp);
        fclose(fp);
    }
}
