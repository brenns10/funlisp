/*
 * Module-related functionality. Includes builtin modules.
 */
#include "funlisp_internal.h"

static void build_example_module(lisp_runtime *rt, lisp_module *m)
{
	lisp_scope_bind(m->contents, lisp_symbol_new(rt, "foo", 0),
			(lisp_value*)lisp_string_new(rt, "bar", 0));
}

lisp_module *create_example_module(lisp_runtime *rt)
{
	lisp_module *m = (lisp_module *) lisp_new(rt, type_module);
	m->name = lisp_string_new(rt, "example", 0);
	m->file = lisp_string_new(rt, __FILE__, 0);
	m->contents = lisp_new_empty_scope(rt);

	build_example_module(rt, m);
	return m;
}
