/*
 * public_util.c: public utility function implementations
 *
 * Contains implementations of simple functions that are only useful to people
 * that aren't peeping into the undefined world of "funlisp_internal.h".
 *
 * Stephen Brennan <stephen@brennan.io>
 */

#include <stdlib.h>
#include <string.h>

#include "funlisp_internal.h"

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

	if (main_func->type == type_error) {
		return NULL;
	}

	args = lisp_list_of_strings(rt, argv, argc, 0);
	args = lisp_quote(rt, args);
	args = lisp_singleton_list(rt, args);
	return lisp_call(rt, scope, main_func, args);
}

lisp_builtin *lisp_builtin_new(lisp_runtime *rt, char *name,
                               lisp_builtin_func call)
{
	lisp_builtin *builtin = (lisp_builtin*)lisp_new(rt, type_builtin);
	builtin->call = call;
	builtin->name = name;
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

lisp_error *lisp_error_new(lisp_runtime *rt, char *message)
{
	lisp_error *err = (lisp_error*)lisp_new(rt, type_error);
	err->message = strdup(message);
	return err;
}

char *lisp_error_get(lisp_error *err)
{
	return err->message;
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
