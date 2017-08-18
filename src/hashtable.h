
/*
 *  hash.h
 *  naive hashtable implementation for the Atto compiler
 *  https://github.com/deveah/atto
 */

#pragma once

#include <stdint.h>

struct hashtable_entry {
  char *key;
  uint64_t value;

  struct hashtable_entry *next;
};

struct hashtable {
  size_t number_of_buckets;
  struct hashtable_entry **buckets;
};

struct hashtable *allocate_hashtable(size_t number_of_buckets);
uint64_t hash_key(struct hashtable *h, char *key);
struct hashtable_entry *allocate_hashtable_entry(char *key, uint64_t value);
void destroy_hashtable(struct hashtable *h);
void hashtable_set(struct hashtable *h, char *key, uint64_t value);
struct hashtable_entry *hashtable_get(struct hashtable *h, char *key);

