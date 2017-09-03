
/*
 *  state.c
 *  part of Atto :: https://github.com/deveah/atto
 */

#include <assert.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include "state.h"

struct atto_state *atto_allocate_state(void)
{
  struct atto_state *a = (struct atto_state *)malloc(sizeof(struct atto_state));
  assert(a != NULL);

  a->number_of_symbols = 0;
  a->number_of_allocated_symbol_slots = ATTO_MINIMUM_ALLOCATED_SYMBOL_SLOTS;
  a->symbol_names = (char **)calloc(a->number_of_allocated_symbol_slots, sizeof(char *));

  a->number_of_global_objects = 0;
  a->number_of_allocated_global_table_slots = ATTO_MINIMUM_ALLOCATED_GLOBAL_TABLE_SLOTS;
  a->global_table = (struct atto_global_table_slot *)calloc(a->number_of_allocated_global_table_slots, sizeof(struct atto_global_table_slot));

  return a;
}

void atto_destroy_object(struct atto_object *o)
{
  if (o->kind == ATTO_OBJECT_KIND_LIST) {
    atto_destroy_object(o->container.list.car);
    atto_destroy_object(o->container.list.cdr);
  }

  if (o->kind == ATTO_OBJECT_KIND_LAMBDA) {
    free(o->container.lambda->instruction_stream);
    free(o->container.lambda);
  }

  free(o);
}

void atto_destroy_state(struct atto_state *a)
{
  uint32_t i;

  for (i = 0; i < a->number_of_symbols; i++) {
    free(a->symbol_names[i]);
  }

  for (i = 0; i < a->number_of_global_objects; i++) {
    atto_destroy_object(a->global_table[i].body);
  }

  free(a->symbol_names);
  free(a->global_table);

  free(a);
}

uint64_t atto_save_symbol(struct atto_state *a, char *name)
{
  char *temp = (char *)malloc(sizeof(char) * (strlen(name) + 1));
  assert(temp != NULL);
  strcpy(temp, name);

  if (a->number_of_symbols == a->number_of_allocated_symbol_slots) {
    a->number_of_allocated_symbol_slots *= 2; /*  that should do it */
    a->symbol_names = (char**)realloc(a->symbol_names, a->number_of_allocated_symbol_slots * sizeof(char *));
  }

  a->symbol_names[a->number_of_symbols] = temp;
  a->number_of_symbols++;

  return (a->number_of_symbols - 1);
}

void atto_save_global_object(struct atto_state *a, char *name, struct atto_object *o)
{
  char *temp = (char *)malloc(sizeof(char) * (strlen(name) + 1));
  assert(temp != NULL);
  strcpy(temp, name);

  if (a->number_of_global_objects == a->number_of_allocated_global_table_slots) {
    a->number_of_allocated_global_table_slots *= 2; /*  that should do it */
    a->global_table = (struct atto_global_table_slot *)realloc(a->global_table, a->number_of_allocated_global_table_slots * sizeof(struct atto_global_table_slot));
  }

  a->global_table[a->number_of_global_objects].name = temp;
  a->global_table[a->number_of_global_objects].body = o;
  a->number_of_global_objects++;
}

