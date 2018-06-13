/*
 * hello_repl.c: A simple read-eval-print loop for funlisp, with builtin
 * functions registered.
 *
 * Stephen Brennan <stephen@brennan.io>
 */

#include <editline/readline.h>
#include <stdio.h>
#include <stdlib.h>

#include "funlisp.h"

/* (1) Here is our builtin function declaration. */
static lisp_value *say_hello(lisp_runtime *rt, lisp_scope *scope,
                             lisp_value *a, void *user)
{
	char *from = user;
	lisp_string *s;
	lisp_value *arglist = lisp_eval_list(rt, scope, a);

	if (!lisp_get_args((lisp_list*)arglist, "S", &s)) {
		return (lisp_value*)lisp_error_new(rt, "expected a string!");
	}

	printf("Hello, %s! I'm %s.\n", lisp_string_get(s), from);

	/* must return something, so return nil */
	return lisp_nil_new(rt);
}

int main(int argc, char **argv)
{
	lisp_runtime *rt = lisp_runtime_new();
	lisp_scope *scope = lisp_new_default_scope(rt);

	(void)argc; /* unused parameters */
	(void)argv;

	/* (2) Here we register the builtin once */
	lisp_scope_add_builtin(rt, scope, "hello", say_hello, "a computer");
	/* (3) Now register the same function with a different context object */
	lisp_scope_add_builtin(rt, scope, "hello_from_stephen", say_hello, "Stephen");

	for (;;) {
		char *input;
		lisp_value *value, *result;

		input = readline("> ");
		if (input == NULL)
			break; /* Ctrl-D, EOF */
		value = lisp_parse(rt, input);
		add_history(input);
		free(input);
		if (!value)
			continue; /* blank line */
		result = lisp_eval(rt, scope, value);
		if (!result) {
			lisp_print_error(rt, stderr);
			lisp_clear_error(rt);
		} else if (!lisp_nil_p(result)) {
			lisp_print(stdout, result);
			fprintf(stdout, "\n");
		}
		lisp_mark(rt, (lisp_value*)scope);
		lisp_sweep(rt);
	}

	lisp_runtime_free(rt);
	return 0;
}
