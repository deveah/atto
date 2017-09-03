
/*
 *  state.h
 *  part of Atto :: https://github.com/deveah/atto
 */

#include <stdint.h>

#include "vm.h"

#pragma once

#define ATTO_MINIMUM_ALLOCATED_SYMBOL_SLOTS       16
#define ATTO_MINIMUM_ALLOCATED_GLOBAL_TABLE_SLOTS 16

struct atto_lambda {
  uint32_t number_of_instructions;
  uint8_t *instruction_stream;
};

struct atto_environment_object {
  char *name;

  #define ATTO_ENVIRONMENT_OBJECT_KIND_GLOBAL   0
  #define ATTO_ENVIRONMENT_OBJECT_KIND_LOCAL    1
  #define ATTO_ENVIRONMENT_OBJECT_KIND_ARGUMENT 2
  uint8_t kind;

  size_t offset;
  struct atto_environment_object *next;
};

struct atto_environment {
  struct atto_environment_object *head;
  struct atto_environment *parent;
};

struct atto_state {
  char **symbol_names;
  uint32_t number_of_symbols;
  uint32_t number_of_allocated_symbol_slots;

  struct atto_vm_state *vm_state;

  struct atto_environment *global_environment;
  size_t global_object_count;
};

struct atto_state *atto_allocate_state(void);

void atto_destroy_object(struct atto_object *o);
void atto_destroy_state(struct atto_state *a);

uint64_t atto_save_symbol(struct atto_state *a, char *name);

void atto_add_to_environment(struct atto_environment *env, char *name, uint8_t kind, size_t offset);
struct atto_environment_object *atto_find_in_environment(struct atto_environment *env, char *name);

struct atto_object *atto_get_object(struct atto_state *a, struct atto_environment_object *eo);

