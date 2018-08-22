/*
 * example_list_append.c: example demonstrating how to append to a list
 *
 * Stephen Brennan <stephen@brennan.io>
 */

#include <stdio.h>

#include "funlisp.h"

int main(int argc, char **argv)
{
	lisp_runtime *rt;
	lisp_list *head, *tail;

	(void) argc; /* unused parameters */
	(void) argv;

	rt = lisp_runtime_new();

	head = tail = (lisp_list*) lisp_nil_new(rt);
	lisp_list_append(rt, &head, &tail, (lisp_value*)lisp_integer_new(rt, 1));
	lisp_list_append(rt, &head, &tail, (lisp_value*)lisp_integer_new(rt, 2));
	lisp_list_append(rt, &head, &tail, (lisp_value*)lisp_integer_new(rt, 3));
	lisp_print(stdout, (lisp_value*)head);
	fprintf(stdout, "\n");

	lisp_runtime_free(rt);
	return 0;
}
