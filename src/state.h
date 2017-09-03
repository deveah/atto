
/*
 *  state.h
 *  part of Atto :: https://github.com/deveah/atto
 */

#include <stdint.h>

#pragma once

#define ATTO_MINIMUM_ALLOCATED_SYMBOL_SLOTS       16
#define ATTO_MINIMUM_ALLOCATED_GLOBAL_TABLE_SLOTS 16

struct atto_lambda {
  uint32_t number_of_instructions;
  uint8_t *instruction_stream;
};

struct atto_object {
  #define ATTO_OBJECT_KIND_NULL   0
  #define ATTO_OBJECT_KIND_NUMBER 1
  #define ATTO_OBJECT_KIND_SYMBOL 2
  #define ATTO_OBJECT_KIND_LIST   3
  #define ATTO_OBJECT_KIND_LAMBDA 4
  uint8_t kind;

  union {
    double number;
    uint64_t symbol;
    struct {
      struct atto_object *car;
      struct atto_object *cdr;
    } list;
    struct atto_lambda *lambda;
  } container;
};

struct atto_global_table_slot {
  char *name;
  struct atto_object *body;
};

struct atto_state {
  char **symbol_names;
  uint32_t number_of_symbols;
  uint32_t number_of_allocated_symbol_slots;

  struct atto_global_table_slot *global_table;
  uint32_t number_of_global_objects;
  uint32_t number_of_allocated_global_table_slots;
};

struct atto_state *atto_allocate_state(void);

void atto_destroy_object(struct atto_object *o);
void atto_destroy_state(struct atto_state *a);

uint64_t atto_save_symbol(struct atto_state *a, char *name);

void atto_save_global_object(struct atto_state *a, char *name, struct atto_object *o);
