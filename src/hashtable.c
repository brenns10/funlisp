/*
 * hashtable.c: a generic hash table with quadratic probing
 *
 * Designed for storing any size key and value. For the common case (when the
 * key and value are simply pointers), a set of _ptr functions are provided.
 *
 * Stephen Brennan <stephen@brennan.io>
 */

#include <string.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

#include "iter.h"
#include "hashtable.h"

/*
 * This is the space we allow for the "marker" in front of each hash
 * table entry (HT_EMPTY, HT_FULL, HT_GRAVE).
 */
#define HTA_KEY_OFFSET 1

#define HASH_TABLE_INITIAL_SIZE 31
#define HASH_TABLE_MAX_LOAD_FACTOR 0.5

#define HT_EMPTY 0
#define HT_FULL 1
#define HT_GRAVE 2

#define nelem(x) (sizeof(x)/sizeof((x)[0]))

/*
 * private functions
 */

static const unsigned long ht_primes[] = {
	31UL, /* 2^5 */
	61UL,
	127UL,
	257UL,
	509UL,
	1021UL,
	2053UL,
	4093UL,
	8191UL,
	16381UL,
	32771UL,
	65537UL,
	131071UL,
	262147UL,
	524287UL,
	1048573UL,
	2097143UL,
	4194301UL,
	8388617UL,
	16777213UL,
	33554467UL,
	67108859UL,
	134217757UL,
	268435459UL,
	536870909UL,
	1073741827UL,
	2147483647UL,
	4294967291UL /* 2^32 */
};

unsigned long binary_search(const unsigned long *array, int len, unsigned long value)
{
	int lo = 0, hi = len, mid;
	while (lo < hi) {
		mid = (lo + hi) / 2;
		if (value <= array[mid]) {
			hi = mid;
		} else {
			lo = mid + 1;
		}
	}
	return lo;
}

/**
 * @brief Returns the next hashtable size.

 * @param current The current size of the hash table.
 * @returns The next size in the sequence for hash tables.
 */
unsigned long ht_next_size(unsigned long current)
{
	unsigned long curridx = binary_search(
			ht_primes, nelem(ht_primes), current);
	return ht_primes[curridx + 1];
}

/*
 * The size of an "item" in the table. This consists of the size of the
 * "marker", the size of the key, and the size of the value.
 */
#define item_size(ht) (HTA_KEY_OFFSET + (ht)->key_size + (ht)->value_size)

/**
 * Return the mark at index i.
 * @param ht Pointer to struct hash_table
 * @param buf Pointer to the actual buffer containing data (usually just
 * ht->table)
 * @param i Logical index of the data
 */
#define mark_at_buf(ht, buf, i) \
	(*(int8_t*) (((char*)buf) + i * item_size(ht)))

/**
 * Return the mark at index i.
 * @param ht Pointer to struct hash table
 * @param i Logical index
 */
#define mark_at(ht, i) mark_at_buf(ht, ht->table, i)

/*
 * Return a pointer to the key at index i.
 */
#define key_ptr_buf(ht, buf, i) \
	(void*) (((char*)buf) + i * item_size(ht) + HTA_KEY_OFFSET)

#define key_ptr(ht, i) key_ptr_buf(ht, (ht)->table, i)
/*
 * Return a pointer to the value at index i.
 */
#define val_ptr_buf(ht, buf, i) \
	(void*) ( \
		((char*)buf) + \
		i * item_size(ht) + \
		HTA_KEY_OFFSET + \
		(ht)->key_size )

#define val_ptr(ht, i) val_ptr_buf(ht, (ht)->table, i)

/**
 * @brief Find the proper index for insertion into the table.
 * @param obj Hash table object.
 * @param key Key we're inserting.
 */
unsigned int ht_find_insert(const struct hashtable *obj, void *key)
{
	unsigned long index = obj->hash(key) % obj->allocated;
	unsigned int j = 1;

	/*
	 * Continue searching until we either find a non-full slot, or we find
	 * the key we're trying to insert:
	 * until (cell.mark != full || cell.key == key)
	 * while (cell.mark == full && cell.key != key)
	 */
	while (mark_at(obj, index) == HT_FULL &&
	       obj->equal(key, key_ptr(obj, index)) != 0) {
		/*
		 * This is quadratic probing, but I'm avoiding squaring numbers:
		 * j:     1, 3, 5, 7,  9, 11, ..
		 * index: 0, 1, 4, 9, 16, 25, 36
		 */
		index = (index + j) % obj->allocated;
		j += 2;
	}

	return index;
}

/**
 * @brief Find the proper index for retrieval from the table.
 * @param obj Hash table object.
 * @param key Key we're looking up.
 */
unsigned int ht_find_retrieve(const struct hashtable *obj, void *key)
{
	unsigned long index = obj->hash(key) % obj->allocated;
	unsigned int j = 1;

	/*
	 * Continue searching until we either find an empty slot, or we find
	 * the key we're trying to insert:
	 * until (cell.mark == empty || cell.key == key)
	 * while (cell.mark != empty && cell.key != key)
	 */
	while (mark_at(obj, index) != HT_EMPTY) {
		/*
		 * When retrieving, we continue whenever we encounter HT_GRAVE,
		 * since items may have been deleted after key was inserted.
		 *
		 * But we MUST NOT interact with any data there, since the
		 * caller expects that we are done with it. It is DEAD DATA to
		 * us.
		 */
		if (mark_at(obj, index) == HT_FULL &&
				obj->equal(key, key_ptr(obj, index)) == 0) {
			return index;
		}
		/*
		 * This is quadratic probing, but I'm avoiding squaring numbers:
		 * j:     1, 3, 5, 7,  9, 11, ..
		 * index: 0, 1, 4, 9, 16, 25, 36
		 */
		index = (index + j) % obj->allocated;
		j += 2;
	}

	return index;
}

/**
 * @brief Expand the hash table, adding increment to the capacity of the table.
 *
 * @param table The table to expand.
 */
void ht_resize(struct hashtable *table)
{
	void *old_table;
	unsigned long index, old_allocated;

	/* Step one: allocate new space for the table */
	old_table = table->table;
	old_allocated = table->allocated;
	table->length = 0;
	table->allocated = ht_next_size(old_allocated);
	table->table = calloc(table->allocated, item_size(table));

	/* Step two, add the old items to the new table. */
	for (index = 0; index < old_allocated; index++) {
		
		if (mark_at_buf(table, old_table, index) == HT_FULL) {
			ht_insert(table, key_ptr_buf(table, old_table, index),
					val_ptr_buf(table, old_table, index));
		}
	}

	/* Step three: free old data. */
	free(old_table);
}

/**
 * @brief Return the load factor of a hash table.
 *
 * @param table The table to find the load factor of.
 * @returns The load factor of the hash table.
 */
double ht_load_factor(struct hashtable *table)
{
	return ((double) table->length) / ((double) table->allocated);
}

/*
 * public functions
 */

void ht_init(struct hashtable *table, hash_t hash_func, comp_t equal,
             unsigned int key_size, unsigned int value_size)
{
	/* Initialize values */
	table->length = 0;
	table->allocated = HASH_TABLE_INITIAL_SIZE;
	table->key_size = key_size;
	table->value_size = value_size;
	table->hash = hash_func;
	table->equal = equal;

	/* Allocate table */
	table->table = calloc(HASH_TABLE_INITIAL_SIZE, item_size(table));
}

struct hashtable *ht_create(hash_t hash_func, comp_t equal,
                            unsigned int key_size, unsigned int value_size)
{
	/* Allocate and create the table. */
	struct hashtable *table;
	table = calloc(sizeof(struct hashtable), 1);
	ht_init(table, hash_func, equal, key_size, value_size);
	return table;
}

void ht_destroy(struct hashtable *table)
{
	free(table->table);
}

void ht_delete(struct hashtable *table)
{
	if (!table) {
		return;
	}

	ht_destroy(table);
	free(table);
}

void ht_insert(struct hashtable *table, void *key, void *value)
{
	unsigned long index;
	if (ht_load_factor(table) > HASH_TABLE_MAX_LOAD_FACTOR) {
		ht_resize(table);
	}

	/*
	 * First, probe for the key as if we're trying to return it. If we find
	 * it, we update the existing key.
	 */
	index = ht_find_retrieve(table, key);
	if (mark_at(table, index) == HT_FULL) {
		memcpy(val_ptr(table, index), value, table->value_size);
		return;
	}

	/*
	 * If we don't find the key, then we find the first open slot or
	 * gravestone.
	 */
	index = ht_find_insert(table, key);
	mark_at(table, index) = HT_FULL;
	memcpy(key_ptr(table, index), key, table->key_size);
	memcpy(val_ptr(table, index), value, table->value_size);
	table->length++;
}

void ht_insert_ptr(struct hashtable *table, void *key, void *value)
{
	ht_insert(table, &key, &value);
}

int ht_remove(struct hashtable *table, void *key)
{
	unsigned long index = ht_find_retrieve(table, key);

	/* If the returned slot isn't full, that means we couldn't find it. */
	if (mark_at(table, index) != HT_FULL) {
		return -1;
	}

	/* Mark the slot with a "grave stone", indicating it is deleted. */
	mark_at(table, index) = HT_GRAVE;
	table->length--;
	return 0;
}

int ht_remove_ptr(struct hashtable *table, void *key)
{
	return ht_remove(table, &key);
}

void *ht_get(struct hashtable const *table, void *key)
{
	unsigned long index = ht_find_retrieve(table, key);

	/* If the slot is not marked full, we didn't find the key. */
	if (mark_at(table, index) != HT_FULL) {
		return NULL;
	}

	/* Otherwise, return the value. */
	return val_ptr(table, index);
}

void *ht_get_ptr(struct hashtable const *table, void *key)
{
	void **result = ht_get(table, &key);
	if (!result) {
		return NULL;
	}
	return *result;
}

void *ht_get_key(struct hashtable const *table, void *key)
{
	unsigned long index = ht_find_retrieve(table, key);

	/* If the slot is not marked full, we didn't find the key. */
	if (mark_at(table, index) != HT_FULL)
		return NULL;

	return key_ptr(table, index);
}

void *ht_get_key_ptr(struct hashtable const *table, void *key)
{
	void **result = ht_get_key(table, &key);
	if (!result)
		return NULL;
	return *result;
}

bool ht_contains(struct hashtable const *table, void *key)
{
	return ht_get(table, key) != NULL;
}

bool ht_contains_ptr(struct hashtable const *table, void *key)
{
	return ht_contains(table, &key);
}

unsigned long ht_length(const struct hashtable *table)
{
	return table->length;
}

unsigned int ht_string_hash(void *data)
{
	char *theString = *(char**)data;
	unsigned int hash = 0;

	while (theString && *theString != '\0' ) {
		hash = (hash << 5) - hash + *theString;
		theString++;
	}

	return hash;
}

int ht_string_comp(void *left, void *right)
{
	char **l = (char**)left, **r = (char**)right;
	return strcmp(*l,*r);
}

int ht_int_comp(void *left, void *right)
{
	int *l = (int*)left, *r = (int*)right;
	return *l - *r;
}

void ht_print(FILE* f, struct hashtable const *table, print_t key, print_t value,
              int full_mode)
{
	unsigned long i;
	char *MARKS[] = {"EMPTY", " FULL", "GRAVE"};

	printf("table size %lu, allocated %lu\n", table->length, table->allocated);
	for (i = 0; i < table->allocated; i++) {
		int8_t mark = mark_at(table, i);
		if (full_mode || mark == HT_FULL) {
			printf("[%04lu|%p|%p|%s]:\n",
					i,
					key_ptr(table, i),
					val_ptr(table, i),
					MARKS[mark]);
			if (mark == HT_FULL) {
				printf("  key: ");
				if (key) key(f, key_ptr(table, i));
				printf("\n  value: ");
				if (value) value(f, val_ptr(table, i));
				printf("\n");
			}
		}
	}
}

static void *ht_next(struct iterator *iter)
{
	/*
	 * This function can be confusing, and it causes difficult-to-debug
	 * issues when you mess it up even a little bit.
	 *
	 * The iterator "next()" operation returns from the next available full
	 * slot, and leaves iter->state_int pointing ONE AFTER that slot (so
	 * that the following call will return from the next available full
	 * slot, etc). For example:
	 *
	 * example table before ht_next() is called (arrow indicates
	 * iter->state_int)
	 *      FULL
	 *   -> EMPTY
	 *      EMPTY
	 *      FULL *
	 *      EMPTY
	 *
	 * afterwards, the starred entry is returned
	 *      FULL
	 *      EMPTY
	 *      EMPTY
	 *      FULL *
	 *   -> EMPTY
	 */
	void *rv;
	struct hashtable *table;
	int8_t mark;

	table = iter->ds;

	while (iter->state_int < (int)table->allocated &&
			mark_at(table, iter->state_int) != HT_FULL) {
		iter->state_int++;
	}
	mark = mark_at(table, iter->state_int);

	if (mark != HT_FULL) return NULL;

	iter->index++;

	if (!iter->state_ptr)
		rv = key_ptr(table, iter->state_int);
	else
		rv = val_ptr(table, iter->state_int);

	iter->state_int++;
	return rv;
}

static void *ht_next_ptr(struct iterator *iter)
{
	void **res = ht_next(iter);
	if (res == NULL)
		return NULL;
	return *res;
}

static bool ht_has_next(struct iterator *iter)
{
	struct hashtable *table = iter->ds;
	return iter->index < (int)table->length;
}

struct iterator ht_iter_keys(struct hashtable *table)
{
	struct iterator it = {0};
	it.ds = table;
	it.index = 0;
	it.state_int = 0;
	it.state_ptr = NULL; /* when null, return keys, else return values */
	it.has_next = ht_has_next;
	it.next = ht_next;
	it.close = iterator_close_noop;
	return it;
}

struct iterator ht_iter_keys_ptr(struct hashtable *table)
{
	struct iterator it = {0};
	it.ds = table;
	it.index = 0;
	it.state_int = 0;
	it.state_ptr = NULL; /* when null, return keys, else return values */
	it.has_next = ht_has_next;
	it.next = ht_next_ptr;
	it.close = iterator_close_noop;
	return it;
}

struct iterator ht_iter_values(struct hashtable *table)
{
	struct iterator it = {0};
	it.ds = table;
	it.index = 0;
	it.state_int = 0;
	it.state_ptr = table; /* when null, return keys, else return values */
	it.has_next = ht_has_next;
	it.next = ht_next;
	it.close = iterator_close_noop;
	return it;
}

struct iterator ht_iter_values_ptr(struct hashtable *table)
{
	struct iterator it = {0};
	it.ds = table;
	it.index = 0;
	it.state_int = 0;
	it.state_ptr = table; /* when null, return keys, else return values */
	it.has_next = ht_has_next;
	it.next = ht_next_ptr;
	it.close = iterator_close_noop;
	return it;
}
