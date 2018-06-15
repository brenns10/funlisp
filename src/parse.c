/*
 * parse.c: recursive descent parser for funlisp
 *
 * Stephen Brennan <stephen@brennan.io>
 */

#include <stdlib.h>
#include <stddef.h>
#include <assert.h>
#include <ctype.h>
#include <string.h>

#include "funlisp_internal.h"
#include "charbuf.h"

typedef struct {
	lisp_value *result;
	int index;
	int error;
} result;

#define return_result_err(v, i, e)                        \
	do {                                              \
		result uncommon_name;                     \
		uncommon_name.result = (lisp_value*) (v); \
		uncommon_name.index = (i);                \
		uncommon_name.error = (e);                \
		return uncommon_name;                     \
	} while(0)

#define return_result(v, i) return_result_err(v, i, 0)

#define COMMENT ';'


static result lisp_parse_value_internal(lisp_runtime *rt, char *input, int index);

static result lisp_parse_integer(lisp_runtime *rt, char *input, int index)
{
	int n, rv;
	lisp_integer *v = (lisp_integer*)lisp_new(rt, type_integer);
	rv = sscanf(input + index, "%d%n", &v->x, &n);
	if (rv != 1) {
		rt->error = "syntax error: error parsing integer";
		return_result_err(NULL, index, 1);
	} else {
		return_result(v, index + n);
	}
}

static int skip_space_and_comments(char *input, int index)
{
	for (;;) {
		while (isspace(input[index])) {
			index++;
		}
		if (input[index] && input[index] == COMMENT) {
			while (input[index] && input[index] != '\n') {
				index++;
			}
		} else {
			return index;
		}
	}
}

static char lisp_escape(char escape)
{
	switch (escape) {
	case 'a':
		return '\a';
	case 'b':
		return '\b';
	case 'f':
		return '\f';
	case 'n':
		return '\n';
	case 'r':
		return '\b';
	case 't':
		return '\t';
	case 'v':
		return '\v';
	default:
		return escape;
	}
}

static result lisp_parse_string(lisp_runtime *rt, char *input, int index)
{
	int i;
	struct charbuf cb;
	lisp_string *str;

	i = index + 1;
	cb_init(&cb, 16);
	while (input[i] && input[i] != '"') {
		if (input[i] == '\\') {
			cb_append(&cb, lisp_escape(input[++i]));
		} else {
			cb_append(&cb, input[i]);
		}
		i++;
	}
	if (!input[i]) {
		cb_destroy(&cb);
		rt->error = "unexpected eof while parsing string";
		return_result_err(NULL, i, 1);
	}
	cb_trim(&cb);
	str = (lisp_string*)lisp_new(rt, type_string);
	str->s = cb.buf;
	i++;
	return_result(str, i);
}

static result lisp_parse_list_or_sexp(lisp_runtime *rt, char *input, int index)
{
	result r;
	lisp_list *rv, *l;

	index = skip_space_and_comments(input, index);
	if (!input[index]) {
		rt->error = "unexpected eof while parsing list";
		return_result_err(NULL, index, 1);
	} else if (input[index] == ')') {
		return_result(lisp_nil_new(rt), index + 1);
	}

	r = lisp_parse_value_internal(rt, input, index);
	if (r.error) return r;
	else if (!r.result) return_result_err(NULL, r.index, 1);
	index = r.index;
	rv = (lisp_list*)lisp_new(rt, type_list);
	rv->left = r.result;
	l = rv;

	while (true) {
		index = skip_space_and_comments(input, index);

		if (!input[index]) {
			rt->error = "unexpected eof while parsing list";
			return_result_err(NULL, index, 1);
		} else if (input[index] == '.') {
			index++;
			r = lisp_parse_value_internal(rt, input, index);
			if (r.error) return r;
			else if (!r.result) return_result_err(NULL, r.index, 1);
			index = r.index;
			l->right = r.result;
			return_result(rv, index);
		} else if (input[index] == ')') {
			index++;
			l->right = lisp_nil_new(rt);
			return_result(rv, index);
		} else {
			r = lisp_parse_value_internal(rt, input, index);
			if (r.error) return r;
			else if (!r.result) return_result_err(NULL, r.index, 1);
			l->right = lisp_new(rt, type_list);
			l = (lisp_list*)l->right;
			l->left = r.result;
			index = r.index;
		}
	}
}

static result lisp_parse_symbol(lisp_runtime *rt, char *input, int index)
{
	int n = 0;
	lisp_symbol *s;

	while (input[index + n] && !isspace(input[index + n]) &&
	       input[index + n] != ')' && input[index + n] != '.' &&
	       input[index + n] != '\'' && input[index + n] != COMMENT) {
		n++;
	}
	if (!input[index]) {
		rt->error = "unexpected eof while parsing symbol";
		return_result_err(NULL, index, 1);
	}
	s = (lisp_symbol*)lisp_new(rt, type_symbol);
	s->sym = malloc(n + 1);
	strncpy(s->sym, input + index, n);
	s->sym[n] = '\0';
	return_result(s, index + n);
}

static result lisp_parse_quote(lisp_runtime *rt, char *input, int index)
{
	result r = lisp_parse_value_internal(rt, input, index + 1);
	if (r.error) return r;
	else if (!r.result) return_result_err(NULL, r.index, 1);
	r.result = lisp_quote(rt, r.result);
	return r;
}

static result lisp_parse_value_internal(lisp_runtime *rt, char *input, int index)
{
	index = skip_space_and_comments(input, index);

	if (input[index] == '"') {
		return lisp_parse_string(rt, input, index);
	}
	if (input[index] == '\0') {
		return_result(NULL, index);
	}
	if (input[index] == ')') {
		return_result(lisp_nil_new(rt), index + 1);
	}
	if (input[index] == '(') {
		return lisp_parse_list_or_sexp(rt, input, index + 1);
	}
	if (input[index] == '\'') {
		return lisp_parse_quote(rt, input, index);
	}
	if (isdigit(input[index])) {
		return lisp_parse_integer(rt, input, index);
	}
	return lisp_parse_symbol(rt, input, index);
}

static void set_error_lineno(lisp_runtime *rt, char *input, int index)
{
	int i;
	rt->error_line = 1;
	for (i = 0; i < index; i++)
		if (input[i] == '\n')
			rt->error_line++;
}

int lisp_parse_value(lisp_runtime *rt, char *input, int index, lisp_value **output)
{
	int bytes;
	result r = lisp_parse_value_internal(rt, input, 0);
	bytes = r.index - index;
	if (r.error) {
		set_error_lineno(rt, input, r.index);
		bytes = -1;
	}
	*output = r.result;
	return bytes;
}

static char *read_file(FILE *input)
{
	size_t bufsize = 1024;
	size_t length = 0;
	char *buf = malloc(1024);

	while (!feof(input) && !ferror(input)) {
		length += fread(buf, sizeof(char), bufsize - length, input);
		if (length >= bufsize) {
			bufsize *= 2;
			buf = realloc(buf, bufsize);
		}
	}

	if (feof(input)) {
		buf[length] = '\0';
		return buf;
	} else {
		free(buf);
		return NULL;
	}
}

lisp_value *lisp_load_file(lisp_runtime *rt, lisp_scope *scope, FILE *input)
{
	char *contents = read_file(input);
	lisp_value *last_val = NULL;
	result r;
	r.index = 0;
	r.result = NULL;

	if (!contents) {
		rt->error = "error reading file or allocating memory";
		return NULL;
	}

	for (;;) {
		r = lisp_parse_value_internal(rt, contents, r.index);
		if (r.error) {
			free(contents);
			return NULL;
		} else if (!r.result) {
			lisp_mark(rt, (lisp_value *) scope);
			if (last_val)
				lisp_mark(rt, last_val);
			lisp_sweep(rt);
			free(contents);
			return last_val;
		}
		last_val = lisp_eval(rt, scope, r.result);
	}
}
