#include "sys/mem/mem.h"

#include <fcntl.h>
#include <stdlib.h> // for malloc
#include <stdio.h>  // for fprintf

int mem_init()
{
    unsigned long size = 1UL << 30;

    dram = malloc(sizeof(char) * size);
    if (!dram) {
        fprintf(stderr, "Could not allocate memory\n");
        return -1;
    }

    return 0;
}
