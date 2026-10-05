#include <pthread.h>

pthread_t *cores;

void *kernel_start(void *);
int cpu_init(pthread_t **_cores, int *num_cores);
