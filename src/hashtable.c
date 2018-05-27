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

#define HTA_KEY_OFFSET 1
#define HASH_TABLE_INITIAL_SIZE 31
#define HASH_TABLE_MAX_LOAD_FACTOR 0.5

#define HT_EMPTY 0
#define HT_FULL 1
#define HT_GRAVE 2

#define HTA_MARK(t, i) ((int8_t*)t->table)[i]

#define nelem(x) (sizeof(x)/sizeof((x)[0]))

/*
 * private functions
 */

unsigned int ht_primes[] = {
  31, // 2^5
  61,
  127,
  257,
  509,
  1021,
  2053,
  4093,
  8191,
  16381,
  32771,
  65537,
  131071,
  262147,
  524287,
  1048573,
  2097143,
  4194301,
  8388617,
  16777213,
  33554467,
  67108859,
  134217757,
  268435459,
  536870909,
  1073741827,
  2147483647,
  4294967291 // 2^32
};

int binary_search(unsigned int *array, int len, unsigned int value)
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
   @brief Returns the next hashtable size.

   @param current The current size of the hash table.
   @returns The next size in the sequence for hash tables.
 */
int ht_next_size(int current)
{
  int curridx = binary_search(ht_primes, nelem(ht_primes), current);
  return ht_primes[curridx + 1];
}

unsigned int item_size(const struct hashtable *obj)
{
  return HTA_KEY_OFFSET + obj->key_size + obj->value_size;
}

unsigned int convert_idx(const struct hashtable *obj, unsigned int orig)
{
  return orig * item_size(obj);
}

/**
   @brief Find the proper index for insertion into the table.
   @param obj Hash table object.
   @param key Key we're inserting.
 */
unsigned int ht_find_insert(const struct hashtable *obj, void *key)
{
  unsigned int index = obj->hash(key) % obj->allocated;
  unsigned int bufidx = convert_idx(obj, index);
  unsigned int j = 1;

  // Continue searching until we either find a non-full slot, or we find the key
  // we're trying to insert.
  // until (cell.mark != full || cell.key == key)
  // while (cell.mark == full && cell.key != key)
  while (HTA_MARK(obj, bufidx) == HT_FULL &&
         obj->equal(key, obj->table + bufidx + HTA_KEY_OFFSET) != 0) {
    // This is quadratic probing, but I'm avoiding squaring numbers:
    // j:     1, 3, 5, 7,  9, 11, ..
    // index: 0, 1, 4, 9, 16, 25, 36
    index = (index + j) % obj->allocated;
    j += 2;
    bufidx = convert_idx(obj, index);
  }

  return index;
}

/**
   @brief Find the proper index for retrieval from the table.
   @param obj Hash table object.
   @param key Key we're looking up.
 */
unsigned int ht_find_retrieve(const struct hashtable *obj, void *key)
{
  unsigned int index = obj->hash(key) % obj->allocated;
  unsigned int bufidx = convert_idx(obj, index);
  unsigned int j = 1;

  // Continue searching until we either find an empty slot, or we find the key
  // we're trying to insert.
  // until (cell.mark == empty || cell.key == key)
  // while (cell.mark != empty && cell.key != key)
  while (HTA_MARK(obj, bufidx) != HT_EMPTY &&
         obj->equal(key, obj->table + bufidx + HTA_KEY_OFFSET) != 0) {
    // This is quadratic probing, but I'm avoiding squaring numbers:
    // j:     1, 3, 5, 7,  9, 11, ..
    // index: 0, 1, 4, 9, 16, 25, 36
    index = (index + j) % obj->allocated;
    j += 2;
    bufidx = convert_idx(obj, index);
  }

  return index;
}

/**
   @brief Expand the hash table, adding increment to the capacity of the table.

   @param table The table to expand.
 */
void ht_resize(struct hashtable *table)
{
  void *old_table;
  unsigned int index, old_allocated, bufidx;

  // Step one: allocate new space for the table
  old_table = table->table;
  old_allocated = table->allocated;
  table->length = 0;
  table->allocated = ht_next_size(old_allocated);
  table->table = calloc(table->allocated, item_size(table));

  // Step two, add the old items to the new table.
  for (index = 0; index < old_allocated; index++) {
    bufidx = convert_idx(table, index);
    if (((int8_t*)old_table)[bufidx] == HT_FULL) {
      ht_insert(table, old_table + bufidx + HTA_KEY_OFFSET,
                 old_table + bufidx + HTA_KEY_OFFSET + table->value_size);
    }
  }

  // Step three: free old data.
  free(old_table);
}

/**
   @brief Return the load factor of a hash table.

   @param table The table to find the load factor of.
   @returns The load factor of the hash table.
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
  // Initialize values
  table->length = 0;
  table->allocated = HASH_TABLE_INITIAL_SIZE;
  table->key_size = key_size;
  table->value_size = value_size;
  table->hash = hash_func;
  table->equal = equal;

  // Allocate table
  table->table = calloc(HASH_TABLE_INITIAL_SIZE, item_size(table));
}

struct hashtable *ht_create(hash_t hash_func, comp_t equal,
                    unsigned int key_size, unsigned int value_size)
{
  // Allocate and create the table.
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
  unsigned int index, bufidx;
  if (ht_load_factor(table) > HASH_TABLE_MAX_LOAD_FACTOR) {
    ht_resize(table);
  }

  // First, probe for the key as if we're trying to return it.  If we find it,
  // we update the existing key.
  index = ht_find_retrieve(table, key);
  bufidx = convert_idx(table, index);
  if (HTA_MARK(table, bufidx) == HT_FULL) {
    memcpy(table->table + bufidx + HTA_KEY_OFFSET + table->key_size, value,
           table->value_size);
    return;
  }

  // If we don't find the key, then we find the first open slot or gravestone.
  index = ht_find_insert(table, key);
  bufidx = convert_idx(table, index);
  HTA_MARK(table, bufidx) = HT_FULL;
  memcpy(table->table + bufidx + HTA_KEY_OFFSET, key, table->key_size);
  memcpy(table->table + bufidx + HTA_KEY_OFFSET + table->key_size, value,
         table->value_size);
  table->length++;
}

void ht_insert_ptr(struct hashtable *table, void *key, void *value)
{
  ht_insert(table, &key, &value);
}

int ht_remove(struct hashtable *table, void *key)
{
  unsigned int index = ht_find_retrieve(table, key);
  unsigned int bufidx = convert_idx(table, index);

  // If the returned slot isn't full, that means we couldn't find it.
  if (HTA_MARK(table, bufidx) != HT_FULL) {
    return -1;
  }

  // Mark the slot with a "grave stone", indicating it is deleted.
  HTA_MARK(table, bufidx) = HT_GRAVE;
  table->length--;
  return 0;
}

int ht_remove_ptr(struct hashtable *table, void *key)
{
  ht_remove(table, &key);
}

void *ht_get(struct hashtable const *table, void *key)
{
  unsigned int index = ht_find_retrieve(table, key);
  unsigned int bufidx = convert_idx(table, index);

  // If the slot is not marked full, we didn't find the key.
  if (HTA_MARK(table, bufidx) != HT_FULL) {
    return NULL;
  }

  // Otherwise, return the value.
  return table->table + bufidx + HTA_KEY_OFFSET + table->key_size;
}

void *ht_get_ptr(struct hashtable const *table, void *key)
{
  void **result = ht_get(table, &key);
  if (!result) {
    return NULL;
  }
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
  unsigned int i, bufidx;
  char *MARKS[] = {"EMPTY", " FULL", "GRAVE"};

  for (i = 0; i < table->allocated; i++) {
    bufidx = convert_idx(table, i);
    int8_t mark = HTA_MARK(table, bufidx);
    if (full_mode || mark == HT_FULL) {
      printf("[%04d|%05d|%s]:\n", i, bufidx, MARKS[mark]);
      if (mark == HT_FULL) {
        printf("  key: ");
        if (key) key(f, table->table + bufidx + HTA_KEY_OFFSET);
        printf("\n  value: ");
        if (value) value(f, table->table + bufidx + HTA_KEY_OFFSET + table->key_size);
        printf("\n");
      }
    }
  }
}

static void *ht_next(struct iterator *iter)
{
  struct hashtable *table = iter->ds;
  unsigned int bufidx;
  int8_t mark = HT_EMPTY;

  for (; mark != HT_FULL && iter->state_int < table->allocated; iter->state_int++) {
    bufidx = convert_idx(table, iter->state_int);
    mark = HTA_MARK(table, bufidx);
  }

  if (mark != HT_FULL) return NULL;

  iter->index++;

  if (!iter->state_ptr)
    return table->table + bufidx + HTA_KEY_OFFSET;
  else
    return table->table + bufidx + HTA_KEY_OFFSET + table->key_size;
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
  return iter->index < table->length;
}

struct iterator ht_iter_keys(struct hashtable *table)
{
  struct iterator it = {
    .ds = table,
    .index = 0,
    .state_int = 0,
    .state_ptr = NULL, /* when null, return keys, else return values */
    .has_next = ht_has_next,
    .next = ht_next,
    .close = iterator_close_noop,
  };
  return it;
}

struct iterator ht_iter_keys_ptr(struct hashtable *table)
{
  struct iterator it = {
    .ds = table,
    .index = 0,
    .state_int = 0,
    .state_ptr = NULL, /* when null, return keys, else return values */
    .has_next = ht_has_next,
    .next = ht_next_ptr,
    .close = iterator_close_noop,
  };
  return it;
}

struct iterator ht_iter_values(struct hashtable *table)
{
  struct iterator it = {
    .ds = table,
    .index = 0,
    .state_int = 0,
    .state_ptr = table, /* when null, return keys, else return values */
    .has_next = ht_has_next,
    .next = ht_next,
    .close = iterator_close_noop,
  };
  return it;
}

struct iterator ht_iter_values_ptr(struct hashtable *table)
{
  struct iterator it = {
    .ds = table,
    .index = 0,
    .state_int = 0,
    .state_ptr = table, /* when null, return keys, else return values */
    .has_next = ht_has_next,
    .next = ht_next_ptr,
    .close = iterator_close_noop,
  };
  return it;
}
