/*
 * lisp.h: private lisp declarations
 *
 * Stephen Brennan <stephen@brennan.io>
 */

#ifndef _FUNLISP_H
#define _FUNLISP_H

#include <stdbool.h>
#include <stdio.h>

#include "iter.h"
#include "ringbuf.h"
#include "hashtable.h"

#define GC_NOMARK 'w'
#define GC_QUEUED 'g'
#define GC_MARKED 'b'

#define LISP_VALUE_HEAD             \
	struct lisp_type  *type;        \
	struct lisp_value *next;        \
	char mark                      \


/* Type declarations. */
typedef struct lisp_value {
	LISP_VALUE_HEAD;
} lisp_value;

/* A lisp_runtime is NOT a lisp_value! */
typedef struct {
	lisp_value *head;
	lisp_value *tail;

	/* Some special values we don't want to lose track of */
	lisp_value *nil;

	struct ringbuf rb;
} lisp_runtime;

/* The below ARE lisp_values! */
typedef struct lisp_scope {
	LISP_VALUE_HEAD;
	struct hashtable scope;
	struct lisp_scope *up;
} lisp_scope;

typedef struct {
	LISP_VALUE_HEAD;
	lisp_value *left;
	lisp_value *right;
} lisp_list;

typedef struct lisp_type {
	LISP_VALUE_HEAD;
	const char *name;
	void (*print)(FILE *f, lisp_value *value);
	lisp_value * (*new)(void);
	void (*free)(void *value);
	struct iterator (*expand)(lisp_value*);
	lisp_value * (*eval)(lisp_runtime *rt, lisp_scope *scope, lisp_value *value);
	lisp_value * (*call)(lisp_runtime *rt, lisp_scope *scope, lisp_value *callable, lisp_value *arg);
} lisp_type;

typedef struct {
	LISP_VALUE_HEAD;
	char *sym;
} lisp_symbol;

typedef struct {
	LISP_VALUE_HEAD;
	char *message;
} lisp_error;

typedef struct {
	LISP_VALUE_HEAD;
	int x;
} lisp_integer;

typedef struct {
	LISP_VALUE_HEAD;
	char *s;
} lisp_string;

typedef lisp_value * (*lisp_builtin_func)(lisp_runtime*, lisp_scope*,lisp_value*);
typedef struct {
	LISP_VALUE_HEAD;
	lisp_builtin_func call;
	char *name;
} lisp_builtin;

typedef struct {
	LISP_VALUE_HEAD;
	lisp_list *args;
	lisp_value *code;
	lisp_scope *closure;
} lisp_lambda;

/* Interpreter stuff */
void lisp_init(lisp_runtime *rt);
void lisp_mark(lisp_runtime *rt, lisp_value *v);
void lisp_sweep(lisp_runtime *rt);
void lisp_destroy(lisp_runtime *rt);

/* Shortcuts for type operations. */
void lisp_print(FILE *f, lisp_value *value);
void lisp_free(lisp_value *value);
lisp_value *lisp_eval(lisp_runtime *rt, lisp_scope *scope, lisp_value *value);
lisp_value *lisp_call(lisp_runtime *rt, lisp_scope *scope, lisp_value *callable,
                      lisp_value *arguments);
lisp_value *lisp_new(lisp_runtime *rt, lisp_type *typ);

/* Shortcuts for creation of objects */
lisp_symbol *lisp_symbol_new(lisp_runtime *rt, char *string);
lisp_error *lisp_error_new(lisp_runtime *rt, char *message);
lisp_builtin *lisp_builtin_new(lisp_runtime *rt, char *name,
                               lisp_builtin_func call);
lisp_value *lisp_nil_new(lisp_runtime *rt);

/* Helper functions */
void lisp_scope_bind(lisp_scope *scope, lisp_symbol *symbol, lisp_value *value);
lisp_value *lisp_scope_lookup(lisp_runtime *rt, lisp_scope *scope,
                              lisp_symbol *symbol);
void lisp_scope_add_builtin(lisp_runtime *rt, lisp_scope *scope, char *name, lisp_builtin_func call);
void lisp_scope_populate_builtins(lisp_runtime *rt, lisp_scope *scope);
lisp_value *lisp_eval_list(lisp_runtime *rt, lisp_scope *scope, lisp_value *list);
lisp_value *lisp_parse(lisp_runtime *rt, char *input);
bool lisp_get_args(lisp_list *list, char *format, ...);
lisp_value *lisp_quote(lisp_runtime *rt, lisp_value *value);
/* List functions */
int lisp_list_length(lisp_list *list);
bool lisp_nil_p(lisp_value *l);

extern lisp_type *type_type;
extern lisp_type *type_scope;
extern lisp_type *type_list;
extern lisp_type *type_symbol;
extern lisp_type *type_error;
extern lisp_type *type_integer;
extern lisp_type *type_string;
extern lisp_type *type_builtin;
extern lisp_type *type_lambda;

#endif
