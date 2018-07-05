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

#include <histedit.h>

#include "funlisp.h"

int disable_symcache = 0;
int disable_strcache = 0;
int line_continue = 0;
extern char **environ;

/**
 * Return a complete line of input from the command line, given an EditLine.
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
static lisp_value *repl_parse_single_input(lisp_runtime *rt, EditLine *el, History *hist)
{
	char *input = NULL;
	char *line;
	int input_len = 0;
	int line_len;
	lisp_value *rv;
	HistEvent ev;

	line_continue = 0;

	for (;;) {
		line = (char*)el_gets(el, &line_len);
		if (line_len <= 0) { /* 0 is EOF, -1 error */
			if (input) free(input);
			return lisp_error(rt, LE_EXIT, "");
		}

		if (!input) {
			/* first time, our input is just the line */
			input = malloc(line_len + 1);
			strncpy(input, line, line_len + 1);
			input_len = line_len;
		} else {
			/* second time, combine previous input with line */
			input = realloc(input, input_len + line_len + 1);
			strncat(input, line, line_len);
			input_len += line_len;
		}

		rv = lisp_parse_progn(rt, input);
		if (rv) {
			/* complete input! */
			history(hist, &ev, H_ENTER, input);
			free(input);
			return rv;
		} else if (lisp_get_errno(rt) != LE_EOF) {
			/* syntax error */
			free(input);
			return NULL;
		}
		/* otherwise, partial line (EOF not expected) */
		line_continue = 1;
	}
}

/*
 * Prompt function - needs to be different for line continuation.
 */
char *repl_prompt(EditLine *e)
{
	(void)e;
	if (line_continue)
		return "...> ";
	else
		return "fun> ";
}

/*
 * Return the filename $HOME/.funlisp_history.
 * Free it when you're done.
 */
char *repl_historyfile(void)
{
	const char *varname = "HOME=";
	const char *basename = ".funlisp_history";
	char *retbuf;
	int varlen = strlen(varname);
	int baselen = strlen(basename);
	int vallen, i;

	for (i = 0; environ[i]; i++)
		if (strncmp(varname, environ[i], varlen) == 0)
			break;

	if (!environ[i]) {
		fprintf(stderr, "Unable to find HOME variable\n");
		exit(1);
	}

	vallen = strlen(&environ[i][varlen]);
	retbuf = malloc(vallen + baselen + 2);
	strncpy(retbuf, &environ[i][varlen], vallen + 1);
	if (retbuf[vallen-1] != '/') {
		strncat(retbuf, "/", 2);
	}
	strncat(retbuf, basename, baselen);

	return retbuf;
}

/*
 * Run a REPL given an already existing lisp_runtime, in case you want to drop
 * into a repl after something else finished running.
 */
void repl_run_with_rt(lisp_runtime *rt, lisp_scope *scope)
{
	HistEvent ev;
	EditLine *el = el_init("funlisp", stdin, stdout, stderr);
	History *hist = history_init();
	char *histfile = repl_historyfile();

	history(hist, &ev, H_SETSIZE, 1000);
	history(hist, &ev, H_LOAD, histfile);
	el_set(el, EL_PROMPT, repl_prompt);
	el_set(el, EL_EDITOR, "emacs");
	el_set(el, EL_HIST, history, hist);

	for (;;) {
		lisp_value *parsed_value, *result;

		parsed_value = repl_parse_single_input(rt, el, hist);
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
	history(hist, &ev, H_SAVE, histfile);
	history_end(hist);
	free(histfile);
	el_end(el);
}

/**
 * Run a REPL :)
 */
int repl_run(void)
{
	lisp_runtime *rt;
	lisp_scope *scope;

	rt = lisp_runtime_new();
	if (!disable_symcache)
		lisp_enable_symcache(rt);
	if (!disable_strcache)
		lisp_enable_strcache(rt);
	scope = lisp_new_default_scope(rt);

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
	if (!disable_symcache)
		lisp_enable_symcache(rt);
	if (!disable_strcache)
		lisp_enable_strcache(rt);
	scope = lisp_new_default_scope(rt);

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
		" -x   When file is specified, load it and run REPL rather than main\n"
		" -T   Disable sTring caching\n"
		" -Y   Disable sYmbol caching"
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
	while ((opt = getopt(argc, argv, "hvxYT")) != -1) {
		switch (opt) {
		case 'x':
			file_repl = 1;
			break;
		case 'v':
			return version();
			break;
		case 'T':
			disable_strcache = 1;
			break;
		case 'Y':
			disable_symcache = 1;
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
