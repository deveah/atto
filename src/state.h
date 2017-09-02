
/*
 *  state.h
 *  part of Atto :: https://github.com/deveah/atto
 */

#include <stdint.h>

#pragma once

#define ATTO_MINIMUM_ALLOCATED_SYMBOL_SLOTS 16

struct atto_state {
  char **symbol_names;
  uint32_t number_of_symbols;
  uint32_t number_of_allocated_symbol_slots;
};

struct atto_state *atto_allocate_state(void);
void atto_destroy_state(struct atto_state *a);

void atto_save_symbol(struct atto_state *a, char *name);

