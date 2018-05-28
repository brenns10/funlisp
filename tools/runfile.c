/*
 * runfile.c: Run a text file containing lisp code
 *
 * Stephen Brennan <stephen@brennan.io>
 */

#include <stdio.h>
#include <stdlib.h>

#include <funlisp.h>


int main(int argc, char **argv)
{
	FILE *input;
	lisp_runtime *rt;
	lisp_scope *scope;
	lisp_value *result;
	int rv;

	if (argc < 2) {
		fprintf(stderr, "error: expected at least one argument\n");
		return EXIT_FAILURE;
	}

	input = fopen(argv[1], "r");
	if (!input) {
		perror("open");
		return EXIT_FAILURE;
	}

	rt = lisp_runtime_new();
	scope = lisp_new_default_scope(rt);

	lisp_load_file(rt, scope, input);
	fclose(input);

	result = lisp_run_main_if_exists(rt, scope, argc - 2, argv + 2);
	if (result && result->type == type_error)
		rv = 1;
	else
		rv = 0;
	lisp_runtime_free(rt); /* sweeps everything before exit */
	return rv;
}
