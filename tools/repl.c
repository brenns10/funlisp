/*
 * repl.c: A simple read-eval-print loop for funlisp
 *
 * Stephen Brennan <stephen@brennan.io>
 */

#include <editline/readline.h>
#include <stdio.h>
#include <stdlib.h>

#include "funlisp.h"

int main(int argc, char **argv)
{
	(void)argc; /* unused parameters */
	(void)argv;

	lisp_runtime *rt = lisp_runtime_new();
	lisp_scope *scope = lisp_new_default_scope(rt);

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
		lisp_print(stdout, result);
		fprintf(stdout, "\n");
		lisp_mark(rt, (lisp_value*)scope);
		lisp_sweep(rt);
	}

	lisp_runtime_free(rt);
	return 0;
}
