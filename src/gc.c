/*
 * gc.c: mark and sweep garbage collection for funlisp
 *
 * Stephen Brennan <stephen@brennan.io>
 */
#include <assert.h>

#include "funlisp_internal.h"

void lisp_init(lisp_runtime *rt)
{
	rt->nil = type_list->new(rt);
	rt->nil->mark = 0;
	rt->nil->type = type_list;
	rt->nil->next = NULL;
	rt->head = rt->nil;
	rt->tail = rt->nil;
	rt->user = NULL;
	rb_init(&rt->rb, sizeof(lisp_value*), 16);
	rt->error= NULL;
	rt->error_line = 0;
	rt->error_stack = NULL;
	rt->stack = (lisp_list *) rt->nil;
	rt->stack_depth = 0;
	rt->symcache = NULL;
	rt->strcache = NULL;
	rt->modules = ht_create(lisp_text_hash, lisp_text_compare,
		sizeof(struct lisp_text *), sizeof(struct lisp_module*));

	lisp_register_module(rt, create_os_module(rt));
}

void lisp_destroy(lisp_runtime *rt)
{
	rt->has_marked = 0; /* ensure we sweep all */
	lisp_sweep(rt);
	rb_destroy(&rt->rb);
	lisp_free(rt, rt->nil);
	if (rt->symcache)
		ht_delete(rt->symcache);
	if (rt->strcache)
		ht_delete(rt->strcache);
	ht_delete(rt->modules);
}

void lisp_mark(lisp_runtime *rt, lisp_value *v)
{
	rb_push_back(&rt->rb, &v);
	rt->has_marked = 1;

	while (rt->rb.count > 0) {
		struct iterator it;

		rb_pop_front(&rt->rb, &v);
		v->mark = GC_MARKED;
		it = v->type->expand(v);
		while (it.has_next(&it)) {
			v = it.next(&it);
			if (v->mark == GC_NOMARK) {
				v->mark = GC_QUEUED;
				rb_push_back(&rt->rb, &v);
			}
		}
		it.close(&it);
	}
}

/*
 * The interpreter contains references to several important objects which we
 * must mark to avoid freeing accidentally. We mark them here. See lisp_sweep()
 * to understand when this is called.
 */
static void lisp_mark_basics(lisp_runtime *rt)
{
	if (rt->error_stack)
		lisp_mark(rt, (lisp_value *) rt->error_stack);
	lisp_mark(rt, (lisp_value *) rt->stack);
}

void lisp_sweep(lisp_runtime *rt)
{
	lisp_value *curr = rt->head;

	/*
	 * When a user has called lisp_mark() before calling lisp_sweep(), we
	 * know that they intend to continue using the interpreter. Conversely,
	 * when a user does not call lisp_mark(), and then calls lisp_sweep(),
	 * we know they are clearing all the interpreter data, and we are safe
	 * to clobber the internal interpreter data.
	 *
	 * So, mark some basic data when stuff has already been marked. But, if
	 * nothing has been marked, then reset internal state and leave the
	 * basic data unmarked.
	 */
	if (rt->has_marked) {
		lisp_mark_basics(rt);
	} else {
		lisp_clear_error(rt);
		rt->stack = (lisp_list*)rt->nil;
		rt->stack_depth = 0;
	}

	while (curr->next) {
		if (curr->next->mark != GC_MARKED) {
			lisp_value *tmp = curr->next->next;
			lisp_free(rt, curr->next);
			curr->next = tmp;
		} else {
			curr->mark = GC_NOMARK;
			curr = curr->next;
		}
	}

	curr->mark = GC_NOMARK;
	rt->tail = curr;
	rt->has_marked = false;
}
