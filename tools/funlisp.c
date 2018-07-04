/*
 * funlisp.c: Full featured REPL. Use this as a more complex usage example, or
 * as your entry point to the language (e.g. python, irb).
 *
 * Although funlisp (the library) intends to be widely compatible, this REPL
 * depends on the editline library, as well as POSIX.2 (for the getopt()
 * function). While it should work on most "normal" Unixes, it may not have the
 * same compatibility as the library itself.
 *
 * Stephen Brennan <stephen@brennan.io>
 */

#define _POSIX_C_SOURCE 2

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <editline/readline.h>

#include "funlisp.h"

/**
 * Return a complete line of input from the command line, using libedit.
 *
 * This function allows the user to compose a multi-line expression (or series
 * of expressions), only returning once the input is complete. It reads lines,
 * adding each line to the current input buffer if the parser returns LE_EOF.
 * All other errors are returned to the caller. If Control-D is sent, we return
 * error LE_EXIT.
 * @param rt runtime
 * @return a fully parsed progn containing maybe an arg
 * @retval NULL on error - use lisp_get_errno() to test the error code
 */
static lisp_value *repl_parse_single_input(lisp_runtime *rt)
{
	char *input = NULL;
	char *line, *tmp;
	int input_len = 0;
	int line_len;
	lisp_value *rv;

	for (;;) {
		line = readline(input ? "  " : "> ");
		if (!line) {
			if (input) free(input);
			return lisp_error(rt, LE_EXIT, "");
		}

		add_history(line);
		line_len = strlen(line);

		if (!input) {
			/* first time, our input is just the line */
			input = line;
			input_len = line_len;
		} else {
			/* second time, combine previous input with line */
			tmp = malloc(line_len + input_len + 2);
			tmp[0] = '\0';
			strncat(tmp, input, input_len + 1);
			strncat(tmp, "\n", 2);
			strncat(tmp, line, line_len + 1);
			free(input);
			free(line);
			input = tmp;
			input_len = input_len + line_len + 1;
		}

		rv = lisp_parse_progn(rt, input);
		if (rv) {
			/* complete input! */
			free(input);
			return rv;
		} else if (lisp_get_errno(rt) != LE_EOF) {
			/* syntax error */
			free(input);
			return NULL;
		}
	}
}

void repl_run_with_rt(lisp_runtime *rt, lisp_scope *scope)
{
	for (;;) {
		lisp_value *parsed_value, *result;

		parsed_value = repl_parse_single_input(rt);
		if (!parsed_value && lisp_get_errno(rt) == LE_EXIT) {
			/* Ctrl-D */
			break;
		} else if (!parsed_value) {
			/* syntax error or other parse error */
			lisp_print_error(rt, stderr);
			lisp_clear_error(rt);

			lisp_mark(rt, (lisp_value *)scope);
			lisp_sweep(rt);
			continue;
		}

		result = lisp_eval(rt, scope, parsed_value);
		if (!result) {
			/* some general error */
			lisp_print_error(rt, stderr);
			lisp_clear_error(rt);
		} else if (!lisp_nil_p(result)) {
			/* code returned something interesting, print it */
			lisp_print(stdout, result);
			fprintf(stdout, "\n");
		}
		lisp_mark(rt, (lisp_value*)scope);
		lisp_sweep(rt);
	}
}

/**
 * Run a REPL :)
 */
int repl_run(void)
{
	lisp_runtime *rt = lisp_runtime_new();
	lisp_scope *scope = lisp_new_default_scope(rt);
	lisp_enable_symcache(rt);
	repl_run_with_rt(rt, scope);
	lisp_runtime_free(rt); /* implicitly sweeps everything */
	return 0;
}

/**
 * Run a file with some arguments.
 */
int file_run(char *name, int argc, char **argv, int repl)
{
	FILE *file;
	lisp_runtime *rt;
	lisp_scope *scope;
	lisp_value *result;
	int rv = 0;

	file = fopen(name, "r");
	if (!file) {
		perror("open");
		return 1;
	}

	rt = lisp_runtime_new();
	scope = lisp_new_default_scope(rt);
	lisp_enable_symcache(rt);

	if (!lisp_load_file(rt, scope, file)) {
		fclose(file);
		lisp_print_error(rt, stderr);
		lisp_runtime_free(rt);
		return -1;
	}
	fclose(file);

	if (repl) {
		repl_run_with_rt(rt, scope);
		lisp_runtime_free(rt);
		return 0;
	}
	result = lisp_run_main_if_exists(rt, scope, argc, argv);
	if (!result) {
		lisp_print_error(rt, stderr);
		rv = 1;
	} else if (lisp_is(result, type_integer)) {
		rv = lisp_integer_get((lisp_integer *) result);
	}
	lisp_runtime_free(rt);
	return rv;
}

int help(void)
{
	puts(
		"Usage: funlisp [options...] [file]  load file and run main\n"
		"   or: funlisp [options...]         run a REPL\n"
		"\n"
		"Options:\n"
		" -h   Show this help message and exit\n"
		" -v   Show the funlisp version and exit\n"
		" -x   When file is specified, load it and run REPL rather than main"
	);
	return 0;
}

int version(void)
{
	printf("funlisp version %s\n", lisp_version);
	return 0;
}

int main(int argc, char **argv)
{
	int opt;
	int file_repl = 0;
	while ((opt = getopt(argc, argv, "hvx")) != -1) {
		switch (opt) {
		case 'x':
			file_repl = 1;
			break;
		case 'v':
			return version();
			break;
		case 'h': /* fall through */
		default:
			return help();
		}
	}

	if (optind >= argc) {
		return repl_run();
	} else {
		return file_run(argv[optind], argc - optind, argv + optind, file_repl);
	}
}
