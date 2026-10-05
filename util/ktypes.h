#pragma once

#define NULL 0UL

typedef unsigned long pid_t; // declare pid_t as an unsigned long integer
typedef void *(*kthread_func_t)(long, void *);
typedef kthread_func_t context_func_t;
