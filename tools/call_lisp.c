/*
 * call_lisp.c: example demonstrating how to call lisp functions from C
 *
 * Stephen Brennan <stephen@brennan.io>
 */

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include "funlisp.h"

int call_double_or_square(lisp_runtime *rt, lisp_scope *scope, int x)
{
	int rv;
	lisp_value *args, *res;
	lisp_value *function = lisp_scope_lookup_string(rt, scope,
		"double_or_square");
	assert(function != NULL);

	args = (lisp_value*) lisp_list_new(rt,
		(lisp_value *) lisp_integer_new(rt, x),
		(lisp_value *) lisp_nil_new(rt));
	res = lisp_call(rt, scope, function, args);
	assert(lisp_is(res, type_integer));
	rv = lisp_integer_get((lisp_integer *) res);
	printf("(double_or_square %d) = %d\n", x, rv);
	return rv;
}

int main(int argc, char **argv)
{
	lisp_runtime *rt;
	lisp_scope *scope;
	lisp_value *code;

	(void) argc; /* unused parameters */
	(void) argv;

	rt = lisp_runtime_new();
	scope = lisp_new_default_scope(rt);
	code = lisp_parse(rt,
		"(define double_or_square"
		"  (lambda (x)"
		"    (if (< x 10)"
		"      (* x x)"
		"      (* x 2))))"
	);
	lisp_eval(rt, scope, code);

	call_double_or_square(rt, scope, 5);
	call_double_or_square(rt, scope, 7);
	call_double_or_square(rt, scope, 9);
	call_double_or_square(rt, scope, 11);
	call_double_or_square(rt, scope, 13);

	lisp_runtime_free(rt);
	return 0;
}
