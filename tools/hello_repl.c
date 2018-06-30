/*
 * hello_repl.c: very basic read-eval-print loop, with builtins
 *
 * Stephen Brennan <stephen@brennan.io>
 */

#include <stdio.h>
#include <stdlib.h>

#include "funlisp.h"

/* (1) Here is our builtin function declaration. */
static lisp_value *say_hello(lisp_runtime *rt, lisp_scope *scope,
                             lisp_value *arglist, void *user)
{
	char *from = user;
	lisp_string *s;
	(void) scope; /* unused */

	if (!lisp_get_args(rt, (lisp_list*)arglist, "S", &s)) {
		return NULL;
	}

	printf("Hello, %s! I'm %s.\n", lisp_string_get(s), from);

	/* must return something, so return nil */
	return lisp_nil_new(rt);
}

int main(int argc, char **argv)
{
	char input[256];
	lisp_runtime *rt = lisp_runtime_new();
	lisp_scope *scope = lisp_new_default_scope(rt);

	(void)argc; /* unused parameters */
	(void)argv;

	/* (2) Here we register the builtin once */
	lisp_scope_add_builtin(rt, scope, "hello", say_hello, "a computer", 1);
	/* (3) Now register the same function with a different context object */
	lisp_scope_add_builtin(rt, scope, "hello_from_stephen", say_hello, "Stephen", 1);

	for (;;) {
		lisp_value *value, *result;
		int bytes;

		printf("> ");
		fflush(stdout);
		if (!fgets(input, sizeof(input), stdin))
			break;

		bytes = lisp_parse_value(rt, input, 0, &value);
		if (bytes < 0) {
			/* parse error */
			lisp_print_error(rt, stderr);
			lisp_clear_error(rt);
			continue;
		} else if (!value) {
			/* empty line */
			continue;
		}
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
