#include "ringbuf.h"

#include <stdlib.h>
#include <string.h>

void rb_init(struct ringbuf *rb, int dsize, int init)
{
	rb->dsize = dsize;
	rb->nalloc = init;
	rb->start = 0;
	rb->count = 0;
	rb->data = calloc(dsize, init);
}

void rb_destroy(struct ringbuf *rb)
{
	free(rb->data);
}

void rb_grow(struct ringbuf *rb)
{
	int i, oldalloc;
	
	oldalloc = rb->nalloc;
	rb->nalloc *= 2;
	rb->data = realloc(rb->data, rb->nalloc * rb->dsize);

	for (i = 0; i < rb->count; i++) {
		int oldindex, newindex;
		oldindex = (rb->start + i) % oldalloc;
		newindex = (rb->start + i) % rb->nalloc;
		if (oldindex != newindex) {
			memcpy((char*) rb->data + newindex * rb->dsize,
			       (char*) rb->data + oldindex * rb->dsize,
			       rb->nalloc);
		}
	}
}

void rb_push_front(struct ringbuf *rb, void *src)
{
	int newstart;

	if (rb->count >= rb->nalloc) {
		rb_grow(rb);
	}

	/* ensure the new start index is still positive */
	newstart = (rb->start + rb->nalloc - 1) % rb->nalloc;
	rb->start = newstart;
	memcpy((char*)rb->data + rb->start * rb->dsize, src, rb->dsize);
	rb->count++;
}

void rb_pop_front(struct ringbuf *rb, void *dst)
{
	int newstart;
	
	newstart = (rb->start + 1) % rb->nalloc;
	memcpy(dst, (char*)rb->data + rb->start * rb->dsize, rb->dsize);
	rb->start = newstart;
	rb->count--;
}

void rb_push_back(struct ringbuf *rb, void *src)
{
	int index;
	if (rb->count >= rb->nalloc) {
		rb_grow(rb);
	}

	index = (rb->start + rb->count) % rb->nalloc;
	memcpy((char*)rb->data + index * rb->dsize, src, rb->dsize);
	rb->count++;
}

void rb_pop_back(struct ringbuf *rb, void *dst)
{
	int index;

	index = (rb->start + rb->count - 1) % rb->nalloc;
	memcpy(dst, (char*)rb->data + index * rb->dsize, rb->dsize);
	rb->count--;
}
