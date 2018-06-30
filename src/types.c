/*
 * types.c: language types for funlisp
 *
 * Stephen Brennan <stephen@brennan.io>
 */
#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "funlisp_internal.h"
#include "iter.h"
#include "hashtable.h"

#define TYPE_HEADER \
	&type_type_obj, \
	NULL, \
	'w'

/*
 * Some generic functions for types
 */

static lisp_value *eval_error(lisp_runtime *rt, lisp_scope *s, lisp_value *v)
{
	(void)s;
	(void)v;
	return lisp_error(rt, LE_NOEVAL, "cannot evaluate this object");
}

static lisp_value *eval_same(lisp_runtime *rt, lisp_scope *s, lisp_value *v)
{
	(void)rt;
	(void)s;
	return v;
}

static lisp_value *call_error(lisp_runtime *rt, lisp_scope *s, lisp_value *c,
                              lisp_value *v)
{
	(void)s;
	(void)c;
	(void)v;
	return lisp_error(rt, LE_NOCALL, "not callable!");
}

static bool has_next_index_lt_state(struct iterator *iter)
{
	return iter->index < iter->state_int;
}

/*
 * type
 */

static void type_print(FILE *f, lisp_value *v);
static lisp_value *type_new(void);

static lisp_type type_type_obj = {
	TYPE_HEADER,
	/* name */ "type",
	/* print */ type_print,
	/* new */ type_new,
	/* free */ free,
	/* expand */ iterator_empty,
	/* eval */ eval_error,
	/* call */ call_error,
};
lisp_type *type_type = &type_type_obj;

static void type_print(FILE *f, lisp_value *v)
{
	lisp_type *value = (lisp_type*) v;
	fprintf(f, "%s", value->name);
}

static lisp_value *type_new(void)
{
	lisp_type *type = malloc(sizeof(lisp_type));
	return (lisp_value*)type;
}

/*
 * scope
 */

static void scope_print(FILE *f, lisp_value*v);
static lisp_value *scope_new(void);
static void scope_free(void *v);
static struct iterator scope_expand(lisp_value *);

static lisp_type type_scope_obj = {
	TYPE_HEADER,
	/* name */ "scope",
	/* print */ scope_print,
	/* new */ scope_new,
	/* free */ scope_free,
	/* expand */ scope_expand,
	/* eval */ eval_error,
	/* call */ call_error,
};
lisp_type *type_scope = &type_scope_obj;

static unsigned int symbol_hash(void *symbol)
{
	lisp_symbol **sym = symbol;
	return ht_string_hash(&(*sym)->sym);
}

static int symbol_compare(void *left, void *right)
{
	lisp_symbol **sym1 = left;
	lisp_symbol **sym2 = right;
	return strcmp((*sym1)->sym, (*sym2)->sym);
}

static lisp_value *scope_new(void)
{
	lisp_scope *scope = malloc(sizeof(lisp_scope));
	scope->up = NULL;
	ht_init(&scope->scope, symbol_hash, symbol_compare, sizeof(void*), sizeof(void*));
	return (lisp_value*)scope;
}

static void scope_free(void *v)
{
	lisp_scope *scope = (lisp_scope*) v;
	ht_destroy(&scope->scope);
	free(scope);
}

static void scope_print(FILE *f, lisp_value *v)
{
	lisp_scope *scope = (lisp_scope*) v;
	struct iterator it = ht_iter_keys_ptr(&scope->scope);
	fprintf(f, "(scope:");
	while (it.has_next(&it)) {
		lisp_value *key = it.next(&it);
		lisp_value *value = ht_get_ptr(&scope->scope, key);
		fprintf(f, " ");
		lisp_print(f, key);
		fprintf(f, ": ");
		lisp_print(f, value);
	}
	fprintf(f, ")");
}

static struct iterator scope_expand(lisp_value *v)
{
	lisp_scope *scope = (lisp_scope *) v;
	if (scope->up) {
		return iterator_concat3(
			iterator_single_value(&scope->up),
			ht_iter_keys_ptr(&scope->scope),
			ht_iter_values_ptr(&scope->scope)
		);
	} else {
		return iterator_concat2(
			ht_iter_keys_ptr(&scope->scope),
			ht_iter_values_ptr(&scope->scope)
		);
	}
}

/*
 * list
 */

static void list_print(FILE *f, lisp_value *v);
static lisp_value *list_new(void);
static lisp_value *list_eval(lisp_runtime*, lisp_scope*, lisp_value*);
static struct iterator list_expand(lisp_value*);

static lisp_type type_list_obj = {
	TYPE_HEADER,
	/* name */ "list",
	/* print */ list_print,
	/* new */ list_new,
	/* free */ free,
	/* expand */ list_expand,
	/* eval */ list_eval,
	/* call */ call_error,
};
lisp_type *type_list = &type_list_obj;

static lisp_value *list_eval(lisp_runtime *rt, lisp_scope *scope, lisp_value *v)
{
	lisp_value *callable;
	lisp_list *list = (lisp_list*) v;

	if (lisp_nil_p(v)) {
		return lisp_error(rt, LE_NOCALL, "Cannot call empty list");
	}

	if (list->right->type != type_list) {
		return lisp_error(rt, LE_NOCALL, "You may not call with an s-expression");
	}
	callable = lisp_eval(rt, scope, list->left);
	lisp_error_check(callable);
	return lisp_call(rt, scope, callable, list->right);
}

static void list_print_internal(FILE *f, lisp_list *list)
{
	if (lisp_nil_p((lisp_value*)list)) {
		return;
	}
	lisp_print(f, list->left);
	if (list->right->type != type_list) {
		fprintf(f, " . ");
		lisp_print(f, list->right);
		return;
	} else if (!lisp_nil_p((lisp_value*)list)) {
		fprintf(f, " ");
		list_print_internal(f, (lisp_list*)list->right);
	}
}

static void list_print(FILE *f, lisp_value *v)
{
	fprintf(f, "(");
	list_print_internal(f, (lisp_list*)v);
	fprintf(f, ")");
}

static lisp_value *list_new(void)
{
	lisp_list *list = malloc(sizeof(lisp_list));
	list->left = NULL;
	list->right = NULL;
	return (lisp_value*) list;
}

int lisp_nil_p(lisp_value *l)
{
	return (l->type == type_list) &&
		(((lisp_list*)l)->right == NULL) &&
		(((lisp_list*)l)->left == NULL);
}

static void *list_expand_next(struct iterator *it)
{
	lisp_list *l = (lisp_list*) it->ds;
	it->index++;
	switch (it->index) {
	case 1:
		return l->left;
	case 2:
		return l->right;
	default:
		return NULL;
	}
}

static bool list_has_next(struct iterator *it)
{
	lisp_value *l = (lisp_value*)it->ds;
	if (lisp_nil_p(l)) {
		return false;
	} else {
		return it->index < it->state_int;
	}
}

static struct iterator list_expand(lisp_value *v)
{
	struct iterator it = {0};
	it.ds = v;
	it.state_int = 2;
	it.index = 0;
	it.next = list_expand_next;
	it.has_next = list_has_next;
	it.close = iterator_close_noop;
	return it;
}

/*
 * symbol
 */

static void symbol_print(FILE *f, lisp_value *v);
static lisp_value *symbol_new(void);
static lisp_value *symbol_eval(lisp_runtime*, lisp_scope*, lisp_value*);
static void symbol_free(void *v);

static lisp_type type_symbol_obj = {
	TYPE_HEADER,
	/* name */ "symbol",
	/* print */ symbol_print,
	/* new */ symbol_new,
	/* free */ symbol_free,
	/* expand */ iterator_empty,
	/* eval */ symbol_eval,
	/* call */ call_error,
};
lisp_type *type_symbol = &type_symbol_obj;

static void symbol_print(FILE *f, lisp_value *v)
{
	lisp_symbol *symbol = (lisp_symbol*) v;
	fprintf(f, "%s", symbol->sym);
}

static lisp_value *symbol_new(void)
{
	lisp_symbol *symbol = malloc(sizeof(lisp_symbol));
	symbol->sym = NULL;
	symbol->can_free = 1;
	return (lisp_value*)symbol;
}

static lisp_value *symbol_eval(lisp_runtime *rt, lisp_scope *scope,
                               lisp_value *value)
{
	lisp_symbol *symbol;
	(void)rt;

	symbol = (lisp_symbol*) value;
	return lisp_scope_lookup(rt, scope, symbol);
}

static void symbol_free(void *v)
{
	lisp_symbol *symbol = (lisp_symbol*) v;
	if (symbol->can_free)
		free(symbol->sym);
	free(symbol);
}

/*
 * integer
 */

static void integer_print(FILE *f, lisp_value *v);
static lisp_value *integer_new(void);

static lisp_type type_integer_obj = {
	TYPE_HEADER,
	/* name */ "integer",
	/* print */ integer_print,
	/* new */ integer_new,
	/* free */ free,
	/* expand */ iterator_empty,
	/* eval */ eval_same,
	/* call */ call_error,
};
lisp_type *type_integer = &type_integer_obj;

static void integer_print(FILE *f, lisp_value *v)
{
	lisp_integer *integer = (lisp_integer*) v;
	fprintf(f, "%d", integer->x);
}

static lisp_value *integer_new(void)
{
	lisp_integer *integer = malloc(sizeof(lisp_integer));
	integer->x = 0;
	return (lisp_value*)integer;
}

/* string */

static void string_print(FILE *f, lisp_value *v);
static lisp_value *string_new(void);
static void string_free(void *v);

static lisp_type type_string_obj = {
	TYPE_HEADER,
	/* name */ "string",
	/* print */ string_print,
	/* new */ string_new,
	/* free */ string_free,
	/* expand */ iterator_empty,
	/* eval */ eval_same,
	/* call */ call_error,
};
lisp_type *type_string = &type_string_obj;

static void string_print(FILE *f, lisp_value *v)
{
	lisp_string *str = (lisp_string*) v;
	fprintf(f, "%s", str->s);
}

static lisp_value *string_new(void)
{
	lisp_string *str = malloc(sizeof(lisp_string));
	str->s = NULL;
	str->can_free = 1;
	return (lisp_value*)str;
}

static void string_free(void *v)
{
	lisp_string *str = (lisp_string*) v;
	if (str->can_free)
		free(str->s);
	free(str);
}

/*
 * builtin
 */

static void builtin_print(FILE *f, lisp_value *v);
static lisp_value *builtin_new(void);
static lisp_value *builtin_call(lisp_runtime *rt, lisp_scope *scope,
                                lisp_value *c, lisp_value *arguments);

static lisp_type type_builtin_obj = {
	TYPE_HEADER,
	/* name */ "builtin",
	/* print */ builtin_print,
	/* new */ builtin_new,
	/* free */ free,
	/* expand */ iterator_empty,
	/* eval */ eval_error,
	/* call */ builtin_call,
};
lisp_type *type_builtin = &type_builtin_obj;

static void builtin_print(FILE *f, lisp_value *v)
{
	lisp_builtin *builtin = (lisp_builtin*) v;
	fprintf(f, "<builtin function %s>", builtin->name);
}

static lisp_value *builtin_new()
{
	lisp_builtin *builtin = malloc(sizeof(lisp_builtin));
	builtin->call = NULL;
	builtin->name = NULL;
	builtin->evald = 0;
	return (lisp_value*) builtin;
}

static lisp_value *builtin_call(lisp_runtime *rt, lisp_scope *scope,
                                lisp_value *c, lisp_value *arguments)
{
	lisp_builtin *builtin = (lisp_builtin*) c;
	if (builtin->evald) {
		/* this is ugly with all the casting but TODO fix it soon */
		arguments = (lisp_value*)lisp_eval_list(rt, scope, (lisp_list*)arguments);
		lisp_error_check(arguments);

	}
	if (arguments->type != type_list) {
		return lisp_error(rt, LE_SYNTAX,
			"unrecognized syntax form, builtin must be "
			"called with list");
	}
	return builtin->call(rt, scope, (lisp_list *) arguments, builtin->user);
}

/*
 * lambda
 */

static void lambda_print(FILE *f, lisp_value *v);
static lisp_value *lambda_new(void);
static lisp_value *lambda_call(lisp_runtime *rt, lisp_scope *scope,
                               lisp_value *c, lisp_value *arguments);
static struct iterator lambda_expand(lisp_value *v);

static lisp_type type_lambda_obj = {
	TYPE_HEADER,
	/* name */ "lambda",
	/* print */ lambda_print,
	/* new */ lambda_new,
	/* free */ free,
	/* expand */ lambda_expand,
	/* eval */ eval_error,
	/* call */ lambda_call,
};
lisp_type *type_lambda = &type_lambda_obj;

static void lambda_print(FILE *f, lisp_value *v)
{
	lisp_lambda *l = (lisp_lambda *) v;
	char *name = l->first_binding ? l->first_binding->sym : "(anonymous)";
	if (l->lambda_type == TP_LAMBDA) {
		fprintf(f, "<lambda %s>", name);
	} else {
		fprintf(f, "<macro %s>", name);
	}
}

static lisp_value *lambda_new()
{
	lisp_lambda *lambda = malloc(sizeof(lisp_lambda));
	lambda->args = NULL;
	lambda->code = NULL;
	lambda->closure = NULL;
	lambda->first_binding = NULL;
	lambda->lambda_type = TP_LAMBDA;
	return (lisp_value*) lambda;
}

static lisp_value *lambda_call(lisp_runtime *rt, lisp_scope *scope,
                               lisp_value *c, lisp_value *arguments)
{
	lisp_lambda *lambda;
	lisp_list *argvalues, *it1, *it2;
	lisp_scope *inner;
	lisp_value *result;

	lambda = (lisp_lambda*) c;
	argvalues = lisp_eval_list(rt, scope, (lisp_list*)arguments);
	lisp_error_check(argvalues);
	inner = (lisp_scope*)lisp_new(rt, type_scope);
	inner->up = lambda->closure;

	it1 = lambda->args;
	it2 = argvalues;
	while (!lisp_nil_p((lisp_value*)it1) && !lisp_nil_p((lisp_value*)it2)) {
		lisp_scope_bind(inner, (lisp_symbol*) it1->left, it2->left);
		it1 = (lisp_list*) it1->right;
		it2 = (lisp_list*) it2->right;
	}

	if (!lisp_nil_p((lisp_value*)it1)) {
		return lisp_error(rt, LE_2FEW, "not enough arguments to lambda call");
	}
	if (!lisp_nil_p((lisp_value*)it2)) {
		return lisp_error(rt, LE_2MANY, "too many arguments to lambda call");
	}

	result = lisp_progn(rt, inner, lambda->code);
	lisp_error_check(result);

	if (lambda->lambda_type == TP_MACRO) {
		/* for macros, we've now evaluated the macro to get code, now
		 * evaluate the code */
		result = lisp_eval(rt, scope, result);
	}
	return result;
}

static void *lambda_expand_next(struct iterator *it)
{
	lisp_lambda *l = (lisp_lambda*)it->ds;
	it->index++;
	switch (it->index) {
	case 1:
		return l->args;
	case 2:
		return l->code;
	case 3:
		return l->closure;
	case 4:
		return l->first_binding;
	default:
		return NULL;
	}
}

static struct iterator lambda_expand(lisp_value *v)
{
	lisp_lambda *l = (lisp_lambda *) v;
	struct iterator it = {0};
	it.ds = v;
	it.state_int = l->first_binding ? 4 : 3;
	it.index = 0;
	it.next = lambda_expand_next;
	it.has_next = has_next_index_lt_state;
	it.close = iterator_close_noop;
	return it;
}

/*
 * some shortcuts for accessing these type methods on lisp values
 */

void lisp_print(FILE *f, lisp_value *value)
{
	value->type->print(f, value);
}

void lisp_free(lisp_value *value)
{
	value->type->free(value);
}

lisp_value *lisp_eval(lisp_runtime *rt, lisp_scope *scope, lisp_value *value)
{
	return value->type->eval(rt, scope, value);
}

lisp_value *lisp_call(lisp_runtime *rt, lisp_scope *scope,
                      lisp_value *callable, lisp_value *args)
{
	lisp_value *rv;
	assert(args->type == type_list); /* soon this will be in the signature */
	/* create new stack frame */
	rt->stack = lisp_list_new(rt, callable, (lisp_value *) rt->stack);
	rt->stack_depth++;

	/* make function call */
	rv = callable->type->call(rt, scope, callable, args);

	/* get rid of stack frame */
	rt->stack = (lisp_list*) rt->stack->right;
	rt->stack_depth--;
	return rv;
}

lisp_value *lisp_new(lisp_runtime *rt, lisp_type *typ)
{
	lisp_value *new = typ->new();
	new->type = typ;
	new->next = NULL;
	new->mark = GC_NOMARK;
	if (rt->head == NULL) {
		rt->head = new;
		rt->tail = new;
	} else {
		rt->tail->next = new;
		rt->tail = new;
	}
	return new;
}
