/*
 * hashtable.h: hash table implementation
 *
 * Stephen Brennan <stephen@brennan.io>
 */
#include <stdio.h>

#include "iter.h"

#ifndef HASHTABLE_H
#define HASHTABLE_H

typedef unsigned int (*hash_t)(void *to_hash);

typedef int (*comp_t)(void *left, void *right);

typedef int (*print_t)(FILE *f, void *data);

struct hashtable
{
	unsigned long length;    /* number of items currently in the table */
	unsigned long allocated; /* number of items allocated */

	unsigned int key_size;
	unsigned int value_size;

	hash_t hash;
	comp_t equal;

	void *table;
};

/**
 * @brief Initialize a hash table in memory already allocated.
 * @param table A pointer to the table to initialize.
 * @param hash_func A hash function for the table.
 * @param equal A comparison function for void pointers
 * @param key_size Size of keys.
 * @param value_size Size of values.
 */
void ht_init(struct hashtable *table, hash_t hash_func, comp_t equal,
             unsigned int key_size, unsigned int value_size);
/**
 * @brief Allocate and initialize a hash table.
 * @param hash_func A function that takes one void* and returns a hash value
 * generated from it.  It should be a good hash function.
 * @param equal A comparison function for void pointers.
 * @param key_size Size of keys.
 * @param value_size Size of values.
 * @returns A pointer to the new hash table.
 */
struct hashtable *ht_create(hash_t hash_func, comp_t equal,
                            unsigned int key_size, unsigned int value_size);
/**
 * @brief Free any resources used by the hash table, but doesn't free the
 * pointer.  Doesn't perform any actions on the data as it is deleted.
 *
 * If pointers are contained within the hash table, they are not freed. Use to
 * specify a deletion action on the hash table.
 * @param table The table to destroy.
 */
void ht_destroy(struct hashtable *table);
/**
 * @brief Free the hash table and its resources.	No pointers contained in the
 * table will be freed.
 * @param table The table to free.
 */
void ht_delete(struct hashtable *table);

/**
 * @brief Insert data into the hash table.
 *
 * Expands the hash table if the load factor is below a threshold.	If the key
 * already exists in the table, then the function will overwrite it with the new
 * data provided.
 * @param table A pointer to the hash table.
 * @param key The key to insert.
 * @param value The value to insert at the key.
 */
void ht_insert(struct hashtable *table, void *key, void *value);
void ht_insert_ptr(struct hashtable *table, void *key, void *value);
/**
 * @brief Remove the key, value pair stored in the hash table.
 *
 * This function does not call a deleter on the stored data.
 * @param table A pointer to the hash table.
 * @param key The key to delete.
 * @return -1 on failure, 0 otherwise
 */
int ht_remove(struct hashtable *table, void *key);
int ht_remove_ptr(struct hashtable *table, void *key);
/**
 * @brief Return the value associated with the key provided.
 * @param table A pointer to the hash table.
 * @param key The key whose value to retrieve.
 * @returns The value associated the key, NULL if not found
 */
void *ht_get(struct hashtable const *table, void *key);
void *ht_get_ptr(struct hashtable const *table, void *key);
/**
 * @brief Return true when a key is contained in the table.
 * @param table A pointer to the hash table.
 * @param key The key to search for.
 * @returns Whether the key is present.
 */
bool ht_contains(struct hashtable const *table, void *key);
bool ht_contains_ptr(struct hashtable const *table, void *key);
/**
 * @brief Return the hash of the data, interpreting it as a string.
 * @param data The string to hash, assuming that the value contained is a char*.
 * @returns The hash value of the string.
 */
unsigned int ht_string_hash(void *data);

int ht_string_comp(void *left, void *right);
int ht_int_comp(void *left, void *right);

struct iterator ht_iter_keys(struct hashtable *table);
struct iterator ht_iter_keys_ptr(struct hashtable *table);
struct iterator ht_iter_values(struct hashtable *table);
struct iterator ht_iter_values_ptr(struct hashtable *table);

#endif
