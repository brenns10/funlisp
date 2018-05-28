/*
 * funlisp.h: public funlisp interface
 *
 * Stephen Brennan <stephen@brennan.io>
 */

#ifndef _FUNLISP_H
#define _FUNLISP_H

#include <stdio.h> /* for FILE* */

/*
 * A runtime object -- you need one to execute any code. Note the typedef.
 * Although I generally prefer no struct typedefs, these names are long enough
 * to merit it.
 */
typedef struct lisp_runtime lisp_runtime;

/*
 * Use these functions to create and free a runtime. You must use
 * lisp_runtime_free() to cleanup every runtime you create.
 */
lisp_runtime *lisp_runtime_new(void);
void lisp_runtime_free(lisp_runtime *);

/*
 * In funlisp, (almost) everything is a lisp_value -- that is, can be cast to a
 * lisp_value * and operated on. Integers, Strings, Code, etc. The only thing
 * which is not a lisp_value is the lisp_runtime.
 */
typedef struct lisp_value {
	struct lisp_type  *type; /* type object, USE ME TO TYPE CHECK */
	struct lisp_value *next; /* don't touch me */
	char mark;               /* don't touch me */
} lisp_value;

/*
 * Here are the lisp_values out there:
 */
typedef struct lisp_type lisp_type;
typedef struct lisp_scope lisp_scope;
typedef struct lisp_list lisp_list;
typedef struct lisp_symbol lisp_symbol;
typedef struct lisp_error lisp_error;
typedef struct lisp_integer lisp_integer;
typedef struct lisp_string lisp_string;
typedef struct lisp_builtin lisp_builtin;
typedef struct lisp_lambda lisp_lambda;

/*
 * Each one has an associated "type" object (which you saw above):
 * You can compare the value->type to these, in order to see what type object
 * you have.
 */
extern lisp_type *type_type;
extern lisp_type *type_scope;
extern lisp_type *type_list;
extern lisp_type *type_symbol;
extern lisp_type *type_error;
extern lisp_type *type_integer;
extern lisp_type *type_string;
extern lisp_type *type_builtin;
extern lisp_type *type_lambda;

/*
 * No matter what type you have, it can be printed. This does NOT print a
 * trailing newline, so as to give you the most flexibility.
 */
void lisp_print(FILE *f, lisp_value *value);

/*
 * Since lisp_values can be code, they can be evaluated on a runtime, in a given
 * scope. They, of course, return values.
 */
lisp_value *lisp_eval(lisp_runtime *rt, lisp_scope *scope, lisp_value *value);

/*
 * You get lisp_value's by parsing strings (or FILEs, TODO).
 */
lisp_value *lisp_parse(lisp_runtime *rt, char *input);

/*
 * To do most of this stuff, you need a scope! Thankfully, you can get a new,
 * default scope (containing the language default builtins) quite easily with
 * this function.
 */
lisp_scope *lisp_new_default_scope(lisp_runtime *rt);

/*
 * You can also get a new, empty scope with:
 */
lisp_scope *lisp_new_empty_scope(lisp_runtime *rt);

/*
 * With the above, you should be able to infer what basic use should look like:
 *
 *   lisp_runtime *rt = lisp_runtime_new();
 *   lisp_scope *scope = lisp_new_default_scope(rt);
 *   lisp_value *parsed = lisp_parse(rt, my_cool_code);
 *   lisp_value *result = lisp_eval(rt, scope, parsed);
 *   lisp_print(stdout, result);
 *
 * However, we're missing the memory management stuff here. How does it all get
 * cleaned up?
 *
 * Funlisp uses a mark-and-sweep garbage collector. It cleverly avoids the
 * cycles in data structures (very common in lisp), and frees up unused
 * lisp_values. However, it does need your help.
 */

/*
 * Each time you want to invoke the garbage collector, you should "mark" all the
 * objects you're interested in keeping. Marking an object marks every object
 * you can access from it, using a breadth-first search (so it could be a quite
 * expensive operation). This makes all the objects safe from the following
 * "sweep" operation, which frees all unmarked objects.
 *
 * The normal, correct think to do after evaluating some code, is to mark the
 * scope. Anything still there should be kept. Anything else can probably be
 * cleaned up.
 *
 * NOTE: be explicit about marking. If you do the following:
 *   lisp_value *result = lisp_eval(rt, scope, some_cool_code);
 *   lisp_mark((lisp_value*)scope);
 * There is a very good chance that `result` will be freed on the next sweep.
 * Yeah.
 */
void lisp_mark(lisp_runtime *rt, lisp_value *v);

/*
 * Now, once you've marked your scope (and any other values you're interested
 * in), sweep everything away!
 */
void lisp_sweep(lisp_runtime *rt);

#endif
