/*
 * public_util.c: public utility function implementations
 *
 * Contains implementations of simple functions that are only useful to people
 * that aren't peeping into the undefined world of "funlisp_internal.h".
 *
 * Stephen Brennan <stephen@brennan.io>
 */

#include <stdlib.h>

#include "funlisp_internal.h"

lisp_runtime *lisp_runtime_new(void)
{
	lisp_runtime *rt = malloc(sizeof(lisp_runtime));
	lisp_init(rt);
	return rt;
}

void lisp_runtime_free(lisp_runtime *rt)
{
	lisp_destroy(rt);
	free(rt);
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
