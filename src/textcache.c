/*
 * textcache.c: routines supporting caching strings and symbols
 *
 * Stephen Brennan <stephen@brennan.io>
 */
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include "funlisp_internal.h"

static struct lisp_text *lisp_textcache_lookup(struct hashtable *cache,
		char *str)
{
	struct lisp_text text;
	text.s = str;
	return ht_get_key_ptr(cache, &text);
}

static void lisp_textcache_save(struct hashtable *cache, struct lisp_text *t)
{
	ht_insert_ptr(cache, t, NULL); /* no value :) */
}

void lisp_textcache_remove(struct hashtable *cache, struct lisp_text *t)
{
	struct lisp_text *existing;
	existing = ht_get_key_ptr(cache, t);
	if (existing == t) {
		/*
		 * The same text object may exist multiple times, and only some
		 * could be cached. We only care if this is the same pointer
		 * value.
		 */
		ht_remove_ptr(cache, t);
	}
}

struct hashtable *lisp_textcache_create(void)
{
	return ht_create(lisp_text_hash, lisp_text_compare, sizeof(struct lisp_text*), 0);
}

static char *my_strdup(char *s)
{
	int len = strlen(s);
	char *new = malloc(len + 1);
	strncpy(new, s, len);
	new[len] = '\0';
	return new;
}

static struct lisp_text *lisp_text_new(lisp_runtime *rt, lisp_type *tp,
		struct hashtable *cache, char *str, int flags)
{
	struct lisp_text *string;

	if (cache) {
		string = lisp_textcache_lookup(cache, str);
		if (string) {
			/* If it's cached, we do not need to actually LS_CPY,
			 * since we will not be using the pointer to the string
			 * provided by the user (essentially, there's also
			 * another copy anyway).
			 *
			 * However, we DO need to respect LS_OWN -- if the user
			 * wants us to take ownership of the string (but not
			 * copy it), then we must exercise our ownership by
			 * freeing the argument, else the string will be leaked.
			 */
			if ((flags & LS_OWN) && !(flags & LS_CPY))
				free(str);

			return string;
		}
	}

	/* Uncached (or no cache), so create new text */
	string = (struct lisp_text *) lisp_new(rt, tp);
	if (flags & LS_CPY)
		str = my_strdup(str);
	string->s = str;
	string->can_free = flags & LS_OWN;

	if (cache) {
		/* since this string was previously uncached, let's save it for
		 * next time */
		lisp_textcache_save(cache, string);
	}

	return string;
}

lisp_string *lisp_string_new(lisp_runtime *rt, char *str, int flags)
{
	return (lisp_string*) lisp_text_new(rt, type_string, rt->strcache, str, flags);
}

lisp_symbol *lisp_symbol_new(lisp_runtime *rt, char *sym, int flags)
{
	return (lisp_symbol*) lisp_text_new(rt, type_symbol, rt->symcache, sym, flags);
}

void lisp_enable_strcache(lisp_runtime *rt)
{
	rt->strcache = lisp_textcache_create();
}

void lisp_enable_symcache(lisp_runtime *rt)
{
	rt->symcache = lisp_textcache_create();
}
void lisp_disable_strcache(lisp_runtime *rt)
{
	ht_delete(rt->strcache);
	rt->strcache = NULL;
}

void lisp_disable_symcache(lisp_runtime *rt)
{
	ht_delete(rt->symcache);
	rt->symcache = NULL;
}
