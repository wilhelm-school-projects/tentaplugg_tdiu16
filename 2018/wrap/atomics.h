#pragma once
#include "os.h"

#if 0
/**
 * Atomiska operationer ekvivalenta med följande kod (under antagandet att operationerna körs atomärt):
 *
 * Notera: 'atomic_swap(int *, int *)' går inte att implementera, så den ser lite annorlunda ut här.
 */

const char *atomic_swap(const char **value, const char *replace) {
	void *old = *value;
	*value = replace;
	return old;
}

const char *compare_and_swap(const char **value, const char *compare,
							 const char *swap) {
	void *old = *value;
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

static inline const char *atomic_swap(const char *volatile *value, const char *replace) {
	return __sync_lock_test_and_set(value, replace);
}

static inline const char *compare_and_swap(const char *volatile *value, const char *compare, const char *swap) {
	return __sync_val_compare_and_swap(value, compare, swap);
}

static inline int atomic_add(int volatile *value, int add) {
	return __sync_fetch_and_add(value, add);
}

static inline int atomic_sub(int volatile *value, int sub) {
	return __sync_fetch_and_sub(value, sub);
}

#endif
