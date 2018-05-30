/*
 * repl.c: A simple read-eval-print loop for funlisp
 *
 * Stephen Brennan <stephen@brennan.io>
 */

#include <editline/readline.h>
#include <stdio.h>
#include <stdlib.h>

#include "funlisp.h"

/* NEW: a builtin function declaration */
static lisp_value *say_hello(lisp_runtime *rt, lisp_scope *scope, lisp_value *a)
{
	lisp_string *s;
	lisp_value *arglist = lisp_eval_list(rt, scope, a);

	if (!lisp_get_args((lisp_list*)arglist, "S", &s)) {
		return (lisp_value*)lisp_error_new(rt, "expected a string!");
	}

	printf("Hello, %s!\n", lisp_string_get(s));

	/* must return something, so return nil */
	return lisp_nil_new(rt);
}

int main(int argc, char **argv)
{
	(void)argc; /* unused parameters */
	(void)argv;

	lisp_runtime *rt = lisp_runtime_new();
	lisp_scope *scope = lisp_new_default_scope(rt);

	lisp_scope_add_builtin(rt, scope, "hello", say_hello);

	for (;;) {
		char *input = readline("> ");
		if (input == NULL)
			break; /* Ctrl-D, EOF */
		lisp_value *value = lisp_parse(rt, input);
		add_history(input);
		free(input);
		if (!value)
			continue; /* blank line */
		lisp_value *result = lisp_eval(rt, scope, value);
		if (!lisp_nil_p(result)) {
			lisp_print(stdout, result);
			fprintf(stdout, "\n");
		}
		lisp_mark(rt, (lisp_value*)scope);
		lisp_sweep(rt);
	}

	lisp_runtime_free(rt);
	return 0;
}
