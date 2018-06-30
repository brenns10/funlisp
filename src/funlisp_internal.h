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

/*
 * WARNING - if you change this, you must update "TYPE_HEADER" in types.c.
 */
#define LISP_VALUE_HEAD                 \
	struct lisp_type  *type;        \
	struct lisp_value *next;        \
	char mark                       \

#define lisp_for_each(list) \
	for (; !lisp_nil_p((lisp_value *) list); list = (lisp_list*) list->right)


/*
 * Type declarations.
 */

struct lisp_value {
	LISP_VALUE_HEAD;
};

/* A lisp_runtime is NOT a lisp_value! */
struct lisp_runtime {
	/* Maintains a list of all lisp values allocated with this runtime, so
	 * that we can do garbage collection with mark-and-sweep.
	 */
	lisp_value *head;
	lisp_value *tail;

	/* This is used as a stack/queue for traversing objects during garbage
	 * collection. It's allocated ahead of time to try to avoid allocating
	 * memory as we do garbage collection
	 */
	struct ringbuf rb;
	int has_marked;

	/* Nil is used so much that we keep a global instance and don't bother
	 * ever freeing it. */
	lisp_value *nil;

	/* Some data the user may want to keep track of. */
	void *user;

	/* Data we use for reporting errors. This is very single-threaded of me,
	 * but it works for now.
	 * REQUIREMENTS:
	 *   - error and errno must both always be non-NULL and non-0
	 *     respectively when an error occurs
	 *   - error_line and error_stack may optionally be set
	 * You can use lisp_error() to set error, errno, and error_stack.
	 * Parsing functions typically use a macro to set error, errno, and
	 * error_line.
	 */
	char *error;
	enum lisp_errno errno;
	unsigned int error_line;
	lisp_list *error_stack;

	/* Maintain a stack as we go, can dump it at any time if we want. */
	lisp_list *stack;
	unsigned int stack_depth;
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

/*
 * WARNING - any changes to this structure requires updating the initializers in
 * src/types.c.
 */
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
	void *user;
	int evald;
};

struct lisp_lambda {
	LISP_VALUE_HEAD;
	lisp_list *args;
	lisp_list *code;
	lisp_scope *closure;
	lisp_symbol *first_binding;
	int lambda_type;
};

#define TP_LAMBDA 0
#define TP_MACRO  1

/* Interpreter stuff */
void lisp_init(lisp_runtime *rt);
void lisp_destroy(lisp_runtime *rt);

/* Shortcuts for type operations. */
void lisp_free(lisp_value *value);
lisp_value *lisp_new(lisp_runtime *rt, lisp_type *typ);

lisp_value *lisp_quote_with(lisp_runtime *rt, lisp_value *value, char *sym);

#endif
