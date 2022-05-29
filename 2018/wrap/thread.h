#pragma once
#include "os.h"

typedef void thread_func(void *aux);
tid_t thread_create(const char *name, int priority, thread_func *fn, void *aux);

// Same as above, but simpler.
tid_t thread_new(thread_func *fn, void *aux);

tid_t thread_current(void);
void thread_exit(void);
void thread_yield(void);

void timer_msleep(unsigned milliseconds);
