/*
 * util.c: utilities for funlisp
 *
 * Stephen Brennan <stephen@brennan.io>
 */

#include <stdarg.h>
#include <string.h>
#include <stdlib.h>

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
			return (lisp_value*)lisp_error_new(rt, "symbol not found in scope");
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
	if (lisp_nil_p(l)) {
		return l;
	}
	lisp_list *list = (lisp_list*) l;
	lisp_list *result = (lisp_list*)lisp_new(rt, type_list);
	result->left = lisp_eval(rt, scope, list->left);
	result->right = lisp_eval_list(rt, scope, list->right);
	return (lisp_value*) result;
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
	case 'e':
		return type_error;
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
