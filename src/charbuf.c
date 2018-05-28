/*
 * charbuf.c: simple resizing character buffer for easy string manipulation
 *
 * Stephen Brennan <stephen@brennan.io>
 */

#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <wchar.h>
#include <string.h>

#include "charbuf.h"

/*
 * character buffer - for "narrow", one byte characters
 */

void cb_init(struct charbuf *obj, int capacity)
{
	/* Initialization logic */
	obj->buf = calloc(sizeof(char), (size_t)capacity);
	obj->buf[0] = '\0';
	obj->capacity = capacity;
	obj->length = 0;
}

struct charbuf *cb_create(int capacity)
{
	struct charbuf *obj = calloc(sizeof(struct charbuf), 1);
	cb_init(obj, capacity);
	return obj;
}

void cb_destroy(struct charbuf *obj)
{
	free(obj->buf);
	obj->buf = NULL;
}

void cb_delete(struct charbuf *obj) {
	cb_destroy(obj);
	free(obj);
}

/**
 * @brief Ensure that the struct charbuf can fit a certain amount of characters.
 * @param obj The struct charbuf to expand (if necessary).
 * @param minsize The minimum size the struct charbuf should be able to fit.
 *
 * Note that minsize should include the NUL byte as part of the character count.
 * Therefore, to ensure that the string "four" fits in the buffer, you would
 * want to run `cb_expand_to_fit(obj, 5)` (assuming the buffer was empty).
 */
static void cb_expand_to_fit(struct charbuf *obj, int minsize)
{
	int newcapacity = obj->capacity;
	while (newcapacity < minsize) {
		newcapacity *= 2;
	}
	if (newcapacity != obj->capacity) {
		obj->buf = realloc(obj->buf, sizeof(char) * newcapacity);
		obj->capacity = newcapacity;
	}
}

void cb_concat(struct charbuf *obj, char *buf)
{
	int length = strlen(buf);
	cb_expand_to_fit(obj, obj->length + length + 1);
	strcpy(obj->buf + obj->length, buf);
	obj->length += length;
}

void cb_append(struct charbuf *obj, char next)
{
	cb_expand_to_fit(obj, obj->length + 2); /* include new character + nul */
	obj->buf[obj->length] = next;
	obj->length++;
	obj->buf[obj->length] = '\0';
}

void cb_trim(struct charbuf *obj)
{
	obj->buf = realloc(obj->buf, sizeof(char) * obj->length + 1);
	obj->capacity = obj->length + 1;
}

void cb_clear(struct charbuf *obj)
{
	obj->buf[0] = '\0';
	obj->length = 0;
}

void cb_vprintf(struct charbuf *obj, char *format, va_list va)
{
	va_list v2;
	int length;
	va_copy(v2, va);

	/* Find the length of the formatted string. */
	length = vsnprintf(NULL, 0, format, va);

	/* Make sure we have enough room for everything. */
	cb_expand_to_fit(obj, obj->length + length + 1);

	/* Put the formatted string into the buffer. */
	vsnprintf(obj->buf + obj->length, length + 1, format, v2);
	va_end(v2);
}

void cb_printf(struct charbuf *obj, char *format, ...)
{
	va_list va;
	va_start(va, format);
	cb_vprintf(obj, format, va);
	va_end(va);  /* Have to va_stop() it when you're done using it. */
}
