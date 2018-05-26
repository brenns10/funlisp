/*
 * iter.c: special iterators
 *
 * Stephen Brennan <stephen@brennan.io>
 */

#include <stdlib.h>
#include <stddef.h>
#include <stdint.h>

#include "iter.h"

void iterator_close_noop(struct iterator *iter) {}

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
  struct iterator it = {
    .ds=value,
    .index=0,
    .has_next=sv_has_next,
    .next=sv_next,
    .close=iterator_close_noop,
  };
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
  struct iterator it = {
    .ds = its,
    .index = 0,
    .state_int = 0,
    .state_ptr = (void*) n,

    .has_next = cc_has_next,
    .next = cc_next,
    .close = cc_close,
  };
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
  struct iterator it = {
    .index=0,
    .next=empty_next,
    .has_next=empty_has_next,
    .close=iterator_close_noop,
  };
  return it;
}
