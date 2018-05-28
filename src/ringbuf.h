/*
 * ringbuf.h: expandable, circular buffer
 *
 * Stephen Brennan <stephen@brennan.io>
 */

#ifndef _RINGBUF_H
#define _RINGBUF_H

/**
 * A ring buffer data structure. This buffer can be inserted into and removed
 * from at either end in constant time, except for memory allocations which may
 * have to occur to expand the buffer. However these always double the buffer
 * size, which means the number of allocations is logarithmic with respect to
 * the number of insertions.
 */
struct ringbuf {

	void *data;
	int dsize;

	int nalloc;
	int start;
	int count;

};

/**
 * @brief Initialize a ring buffer.
 * @param rb Pointer to a ring buffer struct.
 * @param dsize Size of data type to store in ring buffer.
 * @param init Initial amount of space to allocate.
 */
void rb_init(struct ringbuf *rb, int dsize, int init);
/**
 * @brief Free all resources held by the ring buffer.
 * @param rb Pointer to the ring buffer struct.
 */
void rb_destroy(struct ringbuf *rb);
/**
 * @brief Add an item to the front of the ring buffer.	May trigger expansion.
 * @param src Area of memory to read from.
 */
void rb_push_front(struct ringbuf *rb, void *src);
/**
 * @brief Remove an item from the front of the ring buffer.
 * @param dst Area of memory to write resulting data to.
 *
 * Note that behavior is unbefined if you decide to pop from an empty buffer.
 */
void rb_pop_front(struct ringbuf *rb, void *dst);
/**
 * @brief Add an item to the end of the ring buffer.	May trigger expansion.
 * @param src Area of memory to read from.
*/
void rb_push_back(struct ringbuf *rb, void *src);
/**
 * @brief Remove an item from the end of the ring buffer.
 * @param dst Area of memory to write resulting data to.
 *
 * Note that behavior is undefined if you decide to pop from an empty buffer.
*/
void rb_pop_back(struct ringbuf *rb, void *dst);

/**
 * @brief Expand a ring buffer (by doubling its size).
 * @param rb Pointer to ring buffer.
 *
 * Note that this is mostly an internal function, and is exposed in the header
 * for testing purposes. No guarantee is made that its interface will stay the
 * same, or that it will continue to exist.
 */
void rb_grow(struct ringbuf *rb);

#endif
