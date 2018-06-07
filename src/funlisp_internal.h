/*
 * funlisp_internal.h: private lisp declarations
 *
 * Stephen Brennan <stephen@brennan.io>
 */

#ifndef _FUNLISP_INTERNAL_H
#define _FUNLISP_INTERNAL_H

#include <stdbool.h>
#include <stdio.h>

#include "funlisp.h"
#include "iter.h"
#include "ringbuf.h"
#include "hashtable.h"

#define GC_NOMARK 'w'
#define GC_QUEUED 'g'
#define GC_MARKED 'b'

#define LISP_VALUE_HEAD                 \
	struct lisp_type  *type;        \
	struct lisp_value *next;        \
	char mark                       \


/*
 * Type declarations.
 */

struct lisp_value {
	LISP_VALUE_HEAD;
};

/* A lisp_runtime is NOT a lisp_value! */
struct lisp_runtime {
	lisp_value *head;
	lisp_value *tail;
	void *user;

	/* Some special values we don't want to lose track of */
	lisp_value *nil;

	struct ringbuf rb;
};

/* The below ARE lisp_values! */
struct lisp_scope {
	LISP_VALUE_HEAD;
	struct hashtable scope;
	struct lisp_scope *up;
};

struct lisp_list {
	LISP_VALUE_HEAD;
	lisp_value *left;
	lisp_value *right;
};

struct lisp_type {
	LISP_VALUE_HEAD;
	const char *name;
	void (*print)(FILE *f, lisp_value *value);
	lisp_value * (*new)(void);
	void (*free)(void *value);
	struct iterator (*expand)(lisp_value*);
	lisp_value * (*eval)(lisp_runtime *rt, lisp_scope *scope, lisp_value *value);
	lisp_value * (*call)(lisp_runtime *rt, lisp_scope *scope, lisp_value *callable, lisp_value *arg);
};

struct lisp_symbol {
	LISP_VALUE_HEAD;
	char can_free;
	char *sym;
};

struct lisp_error {
	LISP_VALUE_HEAD;
	char *message;
};

struct lisp_integer {
	LISP_VALUE_HEAD;
	int x;
};

struct lisp_string {
	LISP_VALUE_HEAD;
	char can_free;
	char *s;
};

struct lisp_builtin {
	LISP_VALUE_HEAD;
	lisp_builtin_func call;
	char *name;
};

struct lisp_lambda {
	LISP_VALUE_HEAD;
	lisp_list *args;
	lisp_value *code;
	lisp_scope *closure;
};

/* Interpreter stuff */
void lisp_init(lisp_runtime *rt);
void lisp_destroy(lisp_runtime *rt);

/* Shortcuts for type operations. */
void lisp_free(lisp_value *value);
lisp_value *lisp_call(lisp_runtime *rt, lisp_scope *scope, lisp_value *callable,
                      lisp_value *arguments);
lisp_value *lisp_new(lisp_runtime *rt, lisp_type *typ);

#endif
