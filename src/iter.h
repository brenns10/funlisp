/*
 * iter.h: generic iterator interface
 *
 * Stephen Brennan <stephen@brennan.io>
 */
#include <stdbool.h>

#ifndef _ITER_H
#define _ITER_H

struct iterator {
	void *ds;        /* the container data structure */
	int index;    /* zero-based index for the iterator */
	int state_int;   /* some state variables that may help */
	void *state_ptr;

	/* do we have a next item? */
	bool (*has_next)(struct iterator *iter);

	/* return the next item (or null) */
	void *(*next)(struct iterator *iter);

	/* free resources held by the iterator */
	void (*close)(struct iterator *iter);
};

void iterator_close_noop(struct iterator *iter);
struct iterator iterator_empty();
struct iterator iterator_single_value(void *value);

/* concatenate n iterators. takes ownership of the *its pointer */
struct iterator iterator_concat(struct iterator *its, size_t n);

/* these use the above function */
struct iterator iterator_concat2(struct iterator left, struct iterator right);
struct iterator iterator_concat3(
	struct iterator a, struct iterator b, struct iterator c);

/* return an iterator that yields an array */
struct iterator iterator_array(void **array, int len, bool own);
struct iterator iterator_from_args(int n, ...);

#endif
