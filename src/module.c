/*
 * Module-related functionality. Includes builtin modules.
 */
#include <stdlib.h>

#include "funlisp_internal.h"

static lisp_value *lisp_os_getenv(lisp_runtime *rt, lisp_scope *scope,
                                  lisp_list *arguments, void *user)
{
	lisp_string *str;
	char *res;

	/* args evaluated */
	(void) user;
	(void) scope;

	if (!lisp_get_args(rt, arguments, "S", &str))
		return NULL;

	res = getenv(str->s);
	if (res)
		return (lisp_value*) lisp_string_new(rt, res, LS_CPY | LS_OWN);
	else
		return lisp_nil_new(rt);
}

static void build_example_module(lisp_runtime *rt, lisp_module *m)
{
	lisp_scope_add_builtin(rt, m->contents, "getenv", lisp_os_getenv, NULL, 1);
}

lisp_module *create_os_module(lisp_runtime *rt)
{
	lisp_module *m = (lisp_module *) lisp_new(rt, type_module);
	m->name = lisp_string_new(rt, "os", 0);
	m->file = lisp_string_new(rt, __FILE__, 0);
	m->contents = lisp_new_empty_scope(rt);

	build_example_module(rt, m);
	return m;
}

void lisp_register_module(lisp_runtime *rt, lisp_module *m)
{
	ht_insert_ptr(rt->modules, m->name, m);
}

lisp_module *lisp_lookup_module(lisp_runtime *rt, lisp_symbol *name)
{
	return ht_get_ptr(rt->modules, name);
}
