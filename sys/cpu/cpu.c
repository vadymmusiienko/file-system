#include "sys/cpu/cpu.h"
#include "kernel/kmain.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void *kernel_start(void *arg)
{
    kmain();
    return NULL;
}

int cpu_init(pthread_t **_cores, int *num_cores)
{
    FILE *cfg = fopen("config.cfg", "r");
    if (!cfg) {
        fprintf(stderr, "Could not read config.cfg. Make sure that you are running from the correct directory...\n");
        return -1;
    }

    char dummy_buf[256];
    fgets(dummy_buf, 256, cfg); // ignore one line...
    fgets(dummy_buf, 256, cfg); // ignore one more line...
    fgets(dummy_buf, 256, cfg); // ignore even one more line... we'll catch any errors next

    if (fscanf(cfg, "cores : %d", num_cores) != 1) {
        fprintf(stderr, "Error reading number of cores...\n");
        return -1;
    }

    if (fclose(cfg) == EOF) {
        fprintf(stderr, "Error closing config");
    }

    cores = malloc(sizeof(pthread_t) * (*num_cores));
    for (int i = 0; i < *num_cores; i++) {
        if (pthread_create(&cores[i], NULL, kernel_start, NULL) != 0) {
            fprintf(stderr, "Failed to start core!\n");
        }
    }

    *_cores = cores;

    return 0;
}
