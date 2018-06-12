/*
 * util.c: utilities for funlisp
 *
 * Contains utilities which may be useful for library users (such as accessors
 * for struct fields we don't expose in the header), as well as handy helper
 * functions to simplify code internally and externally.
 *
 * Stephen Brennan <stephen@brennan.io>
 */

#include <stdarg.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include "funlisp_internal.h"
#include "hashtable.h"

void lisp_scope_bind(lisp_scope *scope, lisp_symbol *symbol, lisp_value *value)
{
	ht_insert_ptr(&scope->scope, symbol, value);
}

lisp_value *lisp_scope_lookup(lisp_runtime *rt, lisp_scope *scope,
                              lisp_symbol *symbol)
{
	lisp_value *v = ht_get_ptr(&scope->scope, symbol);
	if (!v) {
		if (scope->up) {
			return lisp_scope_lookup(rt, scope->up, symbol);
		} else {
			return lisp_error_new(rt, "symbol not found in scope");
		}
	} else {
		return v;
	}
}

lisp_value *lisp_scope_lookup_string(lisp_runtime *rt, lisp_scope *scope, char *name)
{
	/* a dirty hack but why allocate here? */
	lisp_symbol symbol;
	symbol.type = type_symbol;
	symbol.sym = name;
	return lisp_scope_lookup(rt, scope, &symbol);
}

void lisp_scope_add_builtin(lisp_runtime *rt, lisp_scope *scope, char *name,
                            lisp_builtin_func call, void *user)
{
	lisp_symbol *symbol = lisp_symbol_new(rt, name);
	lisp_builtin *builtin = lisp_builtin_new(rt, name, call, user);
	lisp_scope_bind(scope, symbol, (lisp_value*)builtin);
}

lisp_value *lisp_eval_list(lisp_runtime *rt, lisp_scope *scope, lisp_value *l)
{
	lisp_value *left, *right;
	if (lisp_nil_p(l)) {
		return l;
	}
	lisp_list *list = (lisp_list*) l;

	left = lisp_eval(rt, scope, list->left);
	lisp_error_check(left);

	right = lisp_eval_list(rt, scope, list->right);
	lisp_error_check(right);

	return (lisp_value *) lisp_list_new(rt, left, right);
}

int lisp_list_length(lisp_list *list)
{
	int length = 0;
	while (list->type == type_list && !lisp_nil_p((lisp_value*)list)) {
		length++;
		list = (lisp_list*)list->right;
	}
	return length;
}

lisp_value *lisp_quote(lisp_runtime *rt, lisp_value *value) {
	lisp_list *l = (lisp_list*)lisp_new(rt, type_list);
	lisp_symbol *q = lisp_symbol_new(rt, "quote");
	l->left = (lisp_value*)q;
	lisp_list *s = (lisp_list*) lisp_new(rt, type_list);
	s->right = lisp_nil_new(rt);
	l->right = (lisp_value*)s;
	s->left = value;
	return (lisp_value*)l;
}

static lisp_type *lisp_get_type(char c)
{
	switch (c) {
	case 'd':
		return type_integer;
	case 'l':
		return type_list;
	case 's':
		return type_symbol;
	case 'S':
		return type_string;
	case 'o':
		return type_scope;
	case 'b':
		return type_builtin;
	case 't':
		return type_type;
	}
	return NULL;
}

int lisp_get_args(lisp_list *list, char *format, ...)
{
	va_list va;
	va_start(va, format);
	lisp_value **v;
	while (!lisp_nil_p((lisp_value*)list) && *format != '\0') {
		lisp_type *type = lisp_get_type(*format);
		if (type != NULL && type != list->left->type) {
			return 0;
		}
		v = va_arg(va, lisp_value**);
		*v = list->left;
		list = (lisp_list*)list->right;
		format += 1;
	}
	if (strlen(format) != 0 || !lisp_nil_p((lisp_value*)list)) {
		return 0;
	}
	return 1;
}

lisp_value *lisp_list_of_strings(lisp_runtime *rt, char **list, size_t n, char can_free)
{
	size_t i;
	lisp_list *rv, *l;
	lisp_string *s;

	if (n == 0)
		return lisp_nil_new(rt);

	rv = (lisp_list*) lisp_new(rt, type_list);
	l = rv;

	for (i = 0; i < n; i++) {
		s = (lisp_string *) lisp_new(rt, type_string);
		s->s = list[i];
		s->can_free = can_free;
		l->left = (lisp_value *) s;

		l->right = lisp_new(rt, type_list);
		l = (lisp_list *) l->right;
	}

	l->right = lisp_nil_new(rt);

	return (lisp_value *) rv;
}

lisp_value *lisp_singleton_list(lisp_runtime *rt, lisp_value *entry)
{
	lisp_list *l = (lisp_list *) lisp_new(rt, type_list);
	l->left = entry;
	l->right = lisp_nil_new(rt);
	return (lisp_value *) l;
}

lisp_runtime *lisp_runtime_new(void)
{
	lisp_runtime *rt = malloc(sizeof(lisp_runtime));
	lisp_init(rt);
	return rt;
}

void lisp_runtime_set_ctx(lisp_runtime *rt, void *user)
{
	rt->user = user;
}

void *lisp_runtime_get_ctx(lisp_runtime *rt)
{
	return rt->user;
}

void lisp_runtime_free(lisp_runtime *rt)
{
	lisp_destroy(rt);
	free(rt);
}

int lisp_is(lisp_value *value, lisp_type *type)
{
	return value->type == type;
}

lisp_scope *lisp_new_empty_scope(lisp_runtime *rt)
{
	return (lisp_scope *)lisp_new(rt, type_scope);
}

lisp_scope *lisp_new_default_scope(lisp_runtime *rt)
{
	lisp_scope *scope = lisp_new_empty_scope(rt);
	lisp_scope_populate_builtins(rt, scope);
	return scope;
}

lisp_value *lisp_run_main_if_exists(lisp_runtime *rt, lisp_scope *scope,
                                    int argc, char **argv)
{
	lisp_value *args;
	lisp_value *main_func = lisp_scope_lookup(
		rt, scope, lisp_symbol_new(rt, "main"));

	if (main_func->type == NULL) {
		lisp_clear_error(rt);
		return lisp_nil_new(rt);
	}

	args = lisp_list_of_strings(rt, argv, argc, 0);
	args = lisp_quote(rt, args);
	args = lisp_singleton_list(rt, args);
	return lisp_call(rt, scope, main_func, args);
}

lisp_builtin *lisp_builtin_new(lisp_runtime *rt, char *name,
                               lisp_builtin_func call, void *user)
{
	lisp_builtin *builtin = (lisp_builtin*)lisp_new(rt, type_builtin);
	builtin->call = call;
	builtin->name = name;
	builtin->user = user;
	return builtin;
}

lisp_value *lisp_nil_new(lisp_runtime *rt)
{
	if (rt->nil == NULL) {
		rt->nil = lisp_new(rt, type_list);
	}
	return rt->nil;
}

static char *strdup(char *s)
{
	int len = strlen(s);
	char *new = malloc(len + 1);
	strncpy(new, s, len);
	new[len] = '\0';
	return new;
}

lisp_string *lisp_string_new_unowned(lisp_runtime *rt, char *str)
{
	lisp_string *string = (lisp_string *) lisp_new(rt, type_string);
	string->s = str;
	string->can_free = 0;
	return string;
}

lisp_string *lisp_string_new(lisp_runtime *rt, char *str)
{
	lisp_string *string = (lisp_string *) lisp_new(rt, type_string);
	string->s = strdup(str);
	return string;
}

char *lisp_string_get(lisp_string *s)
{
	return s->s;
}

lisp_symbol *lisp_symbol_new(lisp_runtime *rt, char *sym)
{
	lisp_symbol *err = (lisp_symbol*)lisp_new(rt, type_symbol);
	err->sym = strdup(sym);
	return err;
}

char *lisp_symbol_get(lisp_symbol *sym)
{
	return sym->sym;
}

lisp_list *lisp_list_new(lisp_runtime *rt, lisp_value *left, lisp_value *right)
{
	lisp_list *l = (lisp_list *) lisp_new(rt, type_list);
	l->left = left;
	l->right = right;
	return l;
}

lisp_value *lisp_list_get_left(lisp_list *l)
{
	return l->left;
}

lisp_value *lisp_list_get_right(lisp_list *l)
{
	return l->right;
}

lisp_integer *lisp_integer_new(lisp_runtime *rt, int n)
{
	lisp_integer *integer = (lisp_integer *) lisp_new(rt, type_integer);
	integer->x = n;
	return integer;
}

int lisp_integer_get(lisp_integer *integer)
{
	return integer->x;
}

void lisp_dump_stack(lisp_runtime *rt, lisp_list *stack, FILE *file)
{
	if (!stack)
		stack = rt->stack;

	fprintf(file, "Stack trace (most recent call first):\n");
	while (!lisp_nil_p((lisp_value *) stack)) {
		fprintf(file, "  ");
		lisp_print(file, stack->left);
		fprintf(file, "\n");

		stack = (lisp_list *) stack->right;
	}
}

lisp_value *lisp_error_new(lisp_runtime *rt, char *message)
{
	rt->error = message;
	rt->error_stack = rt->stack;
	return NULL;
}

char *lisp_get_error(lisp_runtime *rt)
{
	return rt->error;
}

void lisp_clear_error(lisp_runtime *rt)
{
	rt->error = NULL;
	rt->error_stack = NULL;
	rt->error_line = 0;
}

void lisp_print_error(lisp_runtime *rt, FILE *file)
{
	if (!rt->error) {
		fprintf(stderr, "BUG: lisp_print_error() expects error, found none\n");
		return;
	}

	if (rt->error_line)
		fprintf(file, "at line %d: ", rt->error_line);

	fprintf(file, "%s\n\n", rt->error);

	if (rt->error_stack)
		lisp_dump_stack(rt, rt->error_stack, file);
}
