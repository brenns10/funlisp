/*
 * charbuf.h: simple resizing character buffer for easy string manipulation
 *
 * Stephen Brennan <stephen@brennan.io>
 */

#ifndef _CHARBUF_H
#define _CHARBUF_H

#include <stdarg.h>
#include <wchar.h>

/**
 * @brief A character buffer utility that is easier to handle than a char*.
 *
 * This character buffer provides an interface for string processing that allows
 * you to be efficient, while not needing to handle nearly as much of the
 * allocations that are necessary. It automatically expands as you add to it.
 */
struct charbuf {
	/**
	 * @brief Buffer pointer.
	 */
	char *buf;
	/**
	 * @brief Allocated size of the buffer.
	 */
	int capacity;
	/**
	 * @brief Length of the string in the buffer.
	 */
	int length;
};

/**
 * @brief Initialize a brand-new character buffer.
 *
 * A buffer of the given capacity is initialized, and given an empty string
 * value.
 * @param obj The struct charbuf to initialize.
 * @param capacity Initial capacity of the buffer.
 */
void cb_init(struct charbuf *obj, int capacity);
/**
 * @brief Allocate and initialize a brand-new character buffer.
 *
 * A buffer of the given capacity is initialized, and given an empty string
 * value.  The struct charbuf struct is also allocated and the pointer returned.
 * @param capacity Initial capacity of the buffer.
 */
struct charbuf *cb_create(int capacity);
/**
 * @brief Deallocate the contents of the string buffer.
 * @param obj The character buffer to deallocate.
 */
void cb_destroy(struct charbuf *obj);
/**
 * @brief Deallocate the contents of, and delete the string buffer.
 * @param obj The character buffer to delete.
 */
void cb_delete(struct charbuf *obj);

/**
 * @brief Concat a string onto the end of the character buffer.
 * @param obj The buffer to concat onto.
 * @param str The string to concat.
 */
void cb_concat(struct charbuf *obj, char *str);
/**
 * @brief Append a character onto the end of the character buffer.
 * @param obj The buffer to append onto.
 * @param next The character to append.
 */
void cb_append(struct charbuf *obj, char next);
/**
 * @brief Reallocate the buffer to the exact size of the contained string.
 * @param obj The buffer to reallocate.
 */
void cb_trim(struct charbuf *obj);
/**
 * @brief Empty the buffer of its contents.
 * @param obj The buffer to clear.
 */
void cb_clear(struct charbuf *obj);
/**
 * @brief Format and print a string onto the end of a character buffer.
 * @param obj The object to print onto.
 * @param format The format string to print.
 * @param ... The arguments to the format string.
 */
void cb_printf(struct charbuf *obj, char *format, ...);
/**
 * @brief Format and print a string onto the struct charbuf using a va_list.
 * @param obj The struct charbuf to print into.
 * @param format The format string to print.
 * @param va The vararg list.
 */
void cb_vprintf(struct charbuf *obj, char *format, va_list va);

#endif
