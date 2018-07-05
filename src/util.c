/*
 * util.c: utilities for funlisp
 *
 * Contains utilities which may be useful for library users (such as accessors
 * for struct fields we don't expose in the header), as well as handy helper
 * functions to simplify code internally and externally.
 *
 * Stephen Brennan <stephen@brennan.io>
 */

#include <stdarg.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include "funlisp_internal.h"
#include "hashtable.h"

const char * const lisp_version = FUNLISP_VERSION;

const char *lisp_error_name[LE_MAX_ERR] = {
	"LE_NONE",
	"LE_ERROR",
	"LE_EOF",
	"LE_SYNTAX",
	"LE_FERROR",
	"LE_2MANY",
	"LE_2FEW",
	"LE_TYPE",
	"LE_NOCALL",
	"LE_NOEVAL",
	"LE_NOTFOUND",
	"LE_EXIT",
	"LE_ASSERT",
	"LE_VALUE",
};

void lisp_scope_bind(lisp_scope *scope, lisp_symbol *symbol, lisp_value *value)
{
	lisp_lambda *l;
	ht_insert_ptr(&scope->scope, symbol, value);

	/* for nicer debugging, record the first name binding for lambdas */
	if (value->type == type_lambda) {
		l = (lisp_lambda *) value;
		if (!l->first_binding) {
			l->first_binding = symbol;
		}
	}
}

lisp_value *lisp_scope_lookup(lisp_runtime *rt, lisp_scope *scope,
                              lisp_symbol *symbol)
{
	lisp_value *v = ht_get_ptr(&scope->scope, symbol);
	if (!v) {
		if (scope->up) {
			return lisp_scope_lookup(rt, scope->up, symbol);
		} else {
			return lisp_error(rt, LE_NOTFOUND, "symbol not found in scope");
		}
	} else {
		return v;
	}
}

lisp_value *lisp_scope_lookup_string(lisp_runtime *rt, lisp_scope *scope, char *name)
{
	/* a dirty hack but why allocate here? */
	lisp_symbol symbol;
	symbol.type = type_symbol;
	symbol.s = name;
	return lisp_scope_lookup(rt, scope, &symbol);
}

void lisp_scope_add_builtin(lisp_runtime *rt, lisp_scope *scope, char *name,
                            lisp_builtin_func call, void *user, int evald)
{
	lisp_symbol *symbol = lisp_symbol_new(rt, name, 0);
	lisp_builtin *builtin = lisp_builtin_new(rt, name, call, user, evald);
	lisp_scope_bind(scope, symbol, (lisp_value*)builtin);
}

lisp_value *lisp_mapper_eval(lisp_runtime *rt, lisp_scope *scope, void *user,
                             lisp_value *input)
{
	(void) user;
	return lisp_eval(rt, scope, input);
}

lisp_list *lisp_eval_list(lisp_runtime *rt, lisp_scope *scope, lisp_list *list)
{
	return lisp_map(rt, scope, NULL, lisp_mapper_eval, list);
}

lisp_value *lisp_progn(lisp_runtime *rt, lisp_scope *scope, lisp_list *l)
{
	lisp_value *v;

	if (lisp_nil_p((lisp_value*)l))
		return lisp_nil_new(rt);

	while (1) {
		v = lisp_eval(rt, scope, l->left);
		lisp_error_check(v);

		if (lisp_nil_p(l->right))
			return v;
		else
			l = (lisp_list *) l->right;
	}
}

int lisp_list_length(lisp_list *list)
{
	int length = 0;
	lisp_for_each(list) {
		length++;
	}
	return length;
}

lisp_list *lisp_quote_with(lisp_runtime *rt, lisp_value *value, char *sym) {
	lisp_list *s, *l;
	lisp_symbol *q;

	l = (lisp_list*)lisp_new(rt, type_list);
	q = lisp_symbol_new(rt, sym, 0);
	l->left = (lisp_value*)q;
	s = (lisp_list*) lisp_new(rt, type_list);
	s->right = lisp_nil_new(rt);
	l->right = (lisp_value*)s;
	s->left = value;
	return l;
}

lisp_list *lisp_quote(lisp_runtime *rt, lisp_value *value) {
	return lisp_quote_with(rt, value, "quote");
}

static lisp_type *lisp_get_type(char c)
{
	switch (c) {
	case 'd':
		return type_integer;
	case 'l':
		return type_list;
	case 's':
		return type_symbol;
	case 'S':
		return type_string;
	case 'o':
		return type_scope;
	case 'b':
		return type_builtin;
	case 't':
		return type_type;
	}
	return NULL;
}

int lisp_get_args(lisp_runtime *rt, lisp_list *list, char *format, ...)
{
	lisp_value **v;
	lisp_type *type;
	va_list va;

	va_start(va, format);
	while (!lisp_nil_p((lisp_value*)list) && *format != '\0') {
		v = va_arg(va, lisp_value**);

		if (*format == 'R') {
			/* R = Rest of arguments. Short circuits the get_args
			 * operation, returning what's left.
			 * NB: control-flow will never reach here if what's left
			 * is nil, so R must have at least one item.
			 */
			*v = (lisp_value *) list;
			return 1;
		}
		type = lisp_get_type(*format);
		if (type != NULL && type != list->left->type) {
			rt->error = "incorrect argument type";
			rt->errno = LE_TYPE;
			return 0;
		}
		*v = list->left;
		list = (lisp_list*)list->right;
		format += 1;
	}
	if (*format != '\0') {
		rt->error = "not enough arguments";
		rt->errno = LE_2FEW;
		return 0;
	} else if(!lisp_nil_p((lisp_value*)list)) {
		rt->error = "too many arguments";
		rt->errno = LE_2MANY;
		return 0;
	}
	return 1;
}

lisp_list *lisp_list_of_strings(lisp_runtime *rt, char **list, size_t n, int flags)
{
	size_t i;
	lisp_list *rv, *l;
	lisp_string *s;

	if (n == 0)
		return (lisp_list*) lisp_nil_new(rt);

	rv = (lisp_list*) lisp_new(rt, type_list);
	l = rv;

	for (i = 0; i < n; i++) {
		s = lisp_string_new(rt, list[i], flags);
		l->left = (lisp_value *) s;

		l->right = lisp_new(rt, type_list);
		l = (lisp_list *) l->right;
	}

	l->right = lisp_nil_new(rt);

	return rv;
}

lisp_list *lisp_singleton_list(lisp_runtime *rt, lisp_value *entry)
{
	lisp_list *l = (lisp_list *) lisp_new(rt, type_list);
	l->left = entry;
	l->right = lisp_nil_new(rt);
	return l;
}

lisp_runtime *lisp_runtime_new(void)
{
	lisp_runtime *rt = malloc(sizeof(lisp_runtime));
	lisp_init(rt);
	return rt;
}

void lisp_runtime_set_ctx(lisp_runtime *rt, void *user)
{
	rt->user = user;
}

void *lisp_runtime_get_ctx(lisp_runtime *rt)
{
	return rt->user;
}

void lisp_runtime_free(lisp_runtime *rt)
{
	lisp_destroy(rt);
	free(rt);
}

int lisp_is(lisp_value *value, lisp_type *type)
{
	return value->type == type;
}

lisp_scope *lisp_new_empty_scope(lisp_runtime *rt)
{
	return (lisp_scope *)lisp_new(rt, type_scope);
}

lisp_scope *lisp_new_default_scope(lisp_runtime *rt)
{
	lisp_scope *scope = lisp_new_empty_scope(rt);
	lisp_scope_populate_builtins(rt, scope);
	return scope;
}

lisp_value *lisp_run_main_if_exists(lisp_runtime *rt, lisp_scope *scope,
                                    int argc, char **argv)
{
	lisp_list *args;
	lisp_value *main_func = lisp_scope_lookup(
		rt, scope, lisp_symbol_new(rt, "main", 0));

	if (main_func == NULL) {
		lisp_clear_error(rt);
		return lisp_nil_new(rt);
	}

	args = lisp_list_of_strings(rt, argv, argc, 0);
	args = lisp_quote(rt, (lisp_value*) args);
	args = lisp_singleton_list(rt, (lisp_value*) args);
	return lisp_call(rt, scope, main_func, args);
}

lisp_builtin *lisp_builtin_new(lisp_runtime *rt, char *name,
                               lisp_builtin_func call, void *user, int evald)
{
	lisp_builtin *builtin = (lisp_builtin*)lisp_new(rt, type_builtin);
	builtin->call = call;
	builtin->name = name;
	builtin->user = user;
	builtin->evald = evald;
	return builtin;
}

lisp_value *lisp_nil_new(lisp_runtime *rt)
{
	if (rt->nil == NULL) {
		rt->nil = lisp_new(rt, type_list);
	}
	return rt->nil;
}

char *lisp_string_get(lisp_string *s)
{
	return s->s;
}

char *lisp_symbol_get(lisp_symbol *sym)
{
	return sym->s;
}

lisp_list *lisp_list_new(lisp_runtime *rt, lisp_value *left, lisp_value *right)
{
	lisp_list *l = (lisp_list *) lisp_new(rt, type_list);
	l->left = left;
	l->right = right;
	return l;
}

lisp_value *lisp_list_get_left(lisp_list *l)
{
	return l->left;
}

lisp_value *lisp_list_get_right(lisp_list *l)
{
	return l->right;
}

lisp_integer *lisp_integer_new(lisp_runtime *rt, int n)
{
	lisp_integer *integer = (lisp_integer *) lisp_new(rt, type_integer);
	integer->x = n;
	return integer;
}

int lisp_integer_get(lisp_integer *integer)
{
	return integer->x;
}

void lisp_dump_stack(lisp_runtime *rt, lisp_list *stack, FILE *file)
{
	if (!stack)
		stack = rt->stack;

	fprintf(file, "Stack trace (most recent call first):\n");
	lisp_for_each(stack) {
		fprintf(file, "  ");
		lisp_print(file, stack->left);
		fprintf(file, "\n");
	}
}

lisp_value *lisp_error(lisp_runtime *rt, enum lisp_errno errno, char *message)
{
	rt->error = message;
	rt->errno = errno;
	rt->error_stack = rt->stack;
	return NULL;
}

char *lisp_get_error(lisp_runtime *rt)
{
	return rt->error;
}

enum lisp_errno lisp_get_errno(lisp_runtime *rt)
{
	return rt->errno;
}

void lisp_clear_error(lisp_runtime *rt)
{
	rt->error = NULL;
	rt->error_stack = NULL;
	rt->error_line = 0;
	rt->errno = 0;
}

void lisp_print_error(lisp_runtime *rt, FILE *file)
{
	if (!rt->error) {
		fprintf(stderr, "BUG: lisp_print_error() expects error, found none\n");
		return;
	}

	if (rt->error_line)
		fprintf(file, "at line %d: ", rt->error_line);

	fprintf(file, "Error %s: %s\n", lisp_error_name[rt->errno], rt->error);

	if (rt->error_stack)
		lisp_dump_stack(rt, rt->error_stack, file);
}

enum lisp_errno lisp_sym_to_errno(lisp_symbol *sym)
{
	int i;
	for (i = 0; i < LE_MAX_ERR; i++)
		if (strcmp(sym->s, lisp_error_name[i]) == 0)
			return (enum lisp_errno) i;
	return LE_MAX_ERR;
}

int lisp_is_bad_list(lisp_list *l)
{
	if (l->type != type_list) return 1;
	lisp_for_each(l) {} /* go to first item which is not an empty list */
	return l->type != type_list;
}

int lisp_is_bad_list_of_lists(lisp_list *l)
{
	if (l->type != type_list) return 1;
	lisp_for_each(l) {
		if (lisp_is_bad_list((lisp_list*)l->left)) {
			return 1;
		}
	}
	return l->type != type_list;
}

lisp_list *lisp_map(lisp_runtime *rt, lisp_scope *scope, void *user,
                     lisp_mapper func, lisp_list *list)
{
	lisp_list *new_head=NULL, *new_node;

	if (lisp_nil_p((lisp_value*) list)) {
		return list;
	}

	lisp_for_each(list) {
		if (!new_head) {
			new_head = (lisp_list*) lisp_new(rt, type_list);
			new_node = new_head;
		} else {
			new_node->right = lisp_new(rt, type_list);
			new_node = (lisp_list*) new_node->right;
		}
		new_node->left = func(rt, scope, user, list->left);
		lisp_error_check(new_node->left);
	}

	if (list->type != type_list) {
		/* badly behaved cons cell in list */
		return (lisp_list*) lisp_error(rt, LE_SYNTAX, "unexpected cons cell in list");
	}
	new_node->right = lisp_nil_new(rt);
	return new_head;
}

int lisp_truthy(lisp_value *v)
{
	return v->type == type_integer && ((lisp_integer*)v)->x;
}
