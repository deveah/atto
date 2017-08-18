
/*
 *  hash.c
 *  naive hashtable implementation for the Atto compiler
 *  https://github.com/deveah/atto
 */

#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include "hashtable.h"

struct hashtable *allocate_hashtable(size_t number_of_buckets)
{
  size_t i = 0;

  struct hashtable *h;
  
  h = (struct hashtable *)malloc(sizeof(struct hashtable));
  assert(h != NULL);

  h->number_of_buckets = number_of_buckets;
  h->buckets = (struct hashtable_entry **)malloc(sizeof(struct hashtable_entry *) * number_of_buckets);

  for (i = 0; i < number_of_buckets; i++) {
    h->buckets[i] = NULL;
  }

  return h;
}

/*
 *  hash a key (char*) using the FNV1 algorithm
 *  https://en.wikipedia.org/wiki/Fowler-Noll-Vo_hash_function
 */
uint64_t hash_key(struct hashtable *h, char *key)
{
  uint64_t result = 0;

  size_t i      = 0;
  size_t length = strlen(key);

  result = 0xcbf29ce484222325ULL;

  for (i = 0; i < length; i++) {
    result = result * 0x100000001b3ULL;
    result = result ^ key[i];
  }

  return result % h->number_of_buckets;
}

struct hashtable_entry *allocate_hashtable_entry(char *key, uint64_t value)
{
  struct hashtable_entry *e = NULL;

  e = (struct hashtable_entry *)malloc(sizeof(struct hashtable_entry));
  assert(e != NULL);

  e->key = (char*)malloc(sizeof(char) * (strlen(key) + 1));
  assert(e->key != NULL);
  strcpy(e->key, key);
  
  e->value = value;
  e->next  = NULL;

  return e;
}

void destroy_hashtable(struct hashtable *h)
{
  size_t i;

  struct hashtable_entry *current = NULL;

  for (i = 0; i < h->number_of_buckets; i++) {
    if (h->buckets[i] == NULL) {
      continue;
    }
    
    current = h->buckets[i];
    while (current) {
      struct hashtable_entry *temp = current->next;
      
      free(current->key);
      free(current);

      current = temp;
    }
  }

  free(h->buckets);
  free(h);
}

void hashtable_set(struct hashtable *h, char *key, uint64_t value)
{
  size_t bucket_index = 0;

  struct hashtable_entry *this    = NULL;
  struct hashtable_entry *current = NULL;

  this = allocate_hashtable_entry(key, value);
  bucket_index = hash_key(h, key);

  current = h->buckets[bucket_index];
  if (current == NULL) {
    h->buckets[bucket_index] = this;
  } else {
    while (current->next != NULL) {
      current = current->next;
    }

    current->next = this;
  }
}

struct hashtable_entry *hashtable_get(struct hashtable *h, char *key)
{
  size_t bucket_index = 0;

  struct hashtable_entry *current = NULL;

  bucket_index = hash_key(h, key);

  current = h->buckets[bucket_index];
  if (current == NULL) {
    return NULL;
  } else {
    while ((current != NULL) &&
           (strcmp(current->key, key) != 0)) {
      current = current->next;
    }
  }

  return current;
}

