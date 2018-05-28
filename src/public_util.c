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
