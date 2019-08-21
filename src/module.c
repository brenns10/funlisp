/*
 * Module-related functionality. Includes builtin modules.
 */
#include <stdlib.h>
#include <string.h>

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

lisp_module *create_os_module(lisp_runtime *rt)
{

	lisp_module *m = lisp_new_module(rt, lisp_string_new(rt, "os", 0),
		lisp_string_new(rt, __FILE__, 0));
	lisp_scope_add_builtin(rt, m->contents, "getenv", lisp_os_getenv, NULL, 1);
	return m;
}

void lisp_register_module(lisp_runtime *rt, lisp_module *m)
{
	/* TODO this depends on symbols and strings hashing to same value and
	 * comparing equal */
	lisp_scope_bind(rt->modules, (lisp_symbol*)m->name, (lisp_value*) m);
}

lisp_module *lisp_lookup_module(lisp_runtime *rt, lisp_symbol *name)
{
	lisp_module *m = (lisp_module*) lisp_scope_lookup(rt, rt->modules, name);

	if (!m) /* not found is not necessarily an error */
		lisp_clear_error(rt);

	return m;
}

lisp_module *lisp_new_module(lisp_runtime *rt, lisp_string *name, lisp_string *file)
{
	lisp_module *m = (lisp_module *)lisp_new(rt, type_module);
	m->name = name;
	m->file = file;
	m->contents = lisp_new_empty_scope(rt);
	return m;
}

lisp_scope *lisp_module_get_scope(lisp_module *module)
{
	return module->contents;
}

lisp_module *lisp_import_file(lisp_runtime *rt, lisp_string *name, lisp_string *file)
{
	FILE *f;
	lisp_scope *builtins = lisp_new_default_scope(rt);
	lisp_scope *modscope = lisp_new_empty_scope(rt);
	lisp_module *module;
	lisp_value *v;
	modscope->up = builtins;

	f = fopen(file->s, "r");
	if (!f) {
		return (lisp_module*) lisp_error(rt, LE_ERRNO, "error opening file for import");
	}

	v = lisp_load_file(rt, modscope, f);
	lisp_error_check(v);

	module = (lisp_module *)lisp_new(rt, type_module);
	module->contents = modscope;
	module->name = name;
	module->file = file;
	lisp_register_module(rt, module);
	return module;
}

lisp_module *lisp_do_import(lisp_runtime *rt, lisp_symbol *name)
{
	lisp_module *m;
	lisp_string *file, *namestr;
	char *filestr;
	int len;

	m = lisp_lookup_module(rt, name);
	if (m)
		return m;

	len = strlen(name->s);
	len += 1 /*nul*/ + 7 /*./ .lisp */;
	filestr = malloc(len);
	sprintf(filestr, "./%s.lisp", name->s);
	file = lisp_string_new(rt, filestr, LS_OWN);
	namestr = lisp_symbol_new(rt, name->s, LS_OWN | LS_CPY);
	return lisp_import_file(rt, namestr, file);
}
