/*
 * iter.c: special iterators
 *
 * Stephen Brennan <stephen@brennan.io>
 */

#include <stdlib.h>
#include <stddef.h>
#include <stdint.h>
#include <stdarg.h>

#include "iter.h"

void iterator_close_noop(struct iterator *iter)
{
	(void)iter;
}

static bool sv_has_next(struct iterator *iter)
{
	return iter->index == 0;
}

static void *sv_next(struct iterator *iter)
{
	iter->index++;
	return iter->ds;
}

struct iterator iterator_single_value(void *value)
{
	struct iterator it = {0};
	it.ds = value;
	it.index = 0;
	it.has_next = sv_has_next;
	it.next = sv_next;
	it.close = iterator_close_noop;
	return it;
}

static bool cc_has_next(struct iterator *iter)
{
	struct iterator *its = iter->ds;
	intptr_t max_iterators = (intptr_t) iter->state_ptr;
	bool has_next;

	while (iter->state_int < max_iterators) {
		has_next = its[iter->state_int].has_next(&its[iter->state_int]);
		if (has_next) {
			return true;
		}

		its[iter->state_int].close(&its[iter->state_int]);
		iter->state_int++;
	}

	return false;
}

static void *cc_next(struct iterator *iter)
{
	struct iterator *its = iter->ds;
	void *result = its[iter->state_int].next(&its[iter->state_int]);
	if (result) {
		iter->index++;
	}
	return result;
}

static void cc_close(struct iterator *iter)
{
	free(iter->ds);
}

struct iterator iterator_concat(struct iterator *its, size_t n)
{
	struct iterator it = {0};
	it.ds = its;
	it.index = 0;
	it.state_int = 0;
	it.state_ptr = (void*) n;

	it.has_next = cc_has_next;
	it.next = cc_next;
	it.close = cc_close;
	return it;
}

struct iterator iterator_concat2(struct iterator left, struct iterator right)
{
	struct iterator *arr = calloc(sizeof(struct iterator), 2);
	arr[0] = left;
	arr[1] = right;
	return iterator_concat(arr, 2);
}

struct iterator iterator_concat3(struct iterator a, struct iterator b,
				 struct iterator c)
{
	struct iterator *arr = calloc(sizeof(struct iterator), 3);
	arr[0] = a;
	arr[1] = b;
	arr[2] = c;
	return iterator_concat(arr, 3);
}

static void *empty_next(struct iterator *iter)
{
	(void)iter;
	return NULL;
}

static bool empty_has_next(struct iterator *iter)
{
	(void)iter;
	return false;
}

struct iterator iterator_empty()
{
	struct iterator it = {0};
	it.index=0;
	it.next=empty_next;
	it.has_next=empty_has_next;
	it.close=iterator_close_noop;
	return it;
}

static bool array_has_next(struct iterator *iter)
{
	return iter->index < iter->state_int;
}

static void *array_next(struct iterator *iter)
{
	return ((void**)iter->ds)[iter->index++];
}

static void array_close(struct iterator *iter)
{
	if ((bool) iter->state_ptr)
		free(iter->ds);
}

struct iterator iterator_array(void **array, int len, bool own)
{
	struct iterator it = {0};
	it.index = 0;
	it.ds = array;
	it.state_int = len;
	it.state_ptr = (void*) own;
	it.has_next = array_has_next;
	it.next = array_next;
	it.close = array_close;
	return it;
}

struct iterator iterator_from_args(int n, ...)
{
	void **array = calloc(sizeof(void*), n);
	va_list va;
	int i;

	va_start(va, n);
	for (i = 0; i < n; i++)
		array[i] = va_arg(va, void*);
	va_end(va);

	return iterator_array(array, n, true);
}
