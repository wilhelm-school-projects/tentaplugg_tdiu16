#pragma once
#include "os.h"

#if 0
/**
 * Atomiska operationer ekvivalenta med följande kod (under antagandet att operationerna körs atomärt):
 *
 * Notera: atomic_swap och compare_and_swap kan användas för pekare också.
 */

int test_and_set(int *value) {
    int old = *value;
    *value = 1;
    return old;
}

int atomic_swap(int *value, int replace) {
    int old = *value;
    *value = replace;
    return old;
}

int compare_and_swap(int *value, int compare, int swap) {
    int old = *value;
    if (old == compare)
        *value = swap;
    return old;
}

int atomic_add(int *value, int add) {
    int old = *value;
    *value += add;
    return old;
}

int atomic_sub(int *value, int add) {
    int old = *value;
    *value -= add;
    return old;
}

#endif

#ifdef POSIX

#define test_and_set(value)                     \
    __sync_fetch_and_or(value, 1)

#define atomic_swap(value, replace)             \
    __sync_lock_test_and_set(value, replace)

#define atomic_swap_ptr(value, replace)         \
    __sync_lock_test_and_set(value, replace)

#define compare_and_swap(value, compare, swap)  \
    __sync_val_compare_and_swap(value, compare, swap)

#define compare_and_swap_ptr(value, compare, swap)      \
    __sync_val_compare_and_swap(value, compare, swap)

#define atomic_add(value, add)                  \
    __sync_fetch_and_add(value, add)

#define atomic_sub(value, sub)                  \
    __sync_fetch_and_sub(value, sub)

#endif


#ifdef WIN32

static int test_and_set(int volatile *value) {
    return _InterlockedOr(value, 1);
}

static int atomic_swap(int volatile *value, int replace) {
    return InterlockedExchange(value, replace);
}

static void *atomic_swap_ptr(void *volatile *value, void *replace) {
    return InterlockedExchangePointer(value, replace);
}

static int compare_and_swap(int volatile *value, int compare, int swap) {
    return InterlockedCompareExchange(value, swap, compare);
}

static void *compare_and_swap_ptr(void *volatile *value, void *compare, void *swap) {
    return InterlockedCompareExchangePointer(value, swap, compare);
}

static int atomic_add(int volatile *value, int add) {
    return InterlockedExchangeAdd(value, add);
}

static int atomic_sub(int volatile *value, int sub) {
    return InterlockedExchangeSubtract(value, sub);
}

#endif
