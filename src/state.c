
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

  return a;
}

void atto_destroy_state(struct atto_state *a)
{
  uint32_t i;

  for (i = 0; i < a->number_of_symbols; i++) {
    free(a->symbol_names[i]);
  }

  free(a->symbol_names);
  free(a);
}

void atto_save_symbol(struct atto_state *a, char *name)
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
}

