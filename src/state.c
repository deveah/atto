
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

  a->global_environment = (struct atto_environment *)malloc(sizeof(struct atto_environment));
  assert(a->global_environment != NULL);
  a->global_environment->head = NULL;
  a->global_environment->parent = NULL;

  return a;
}

void atto_destroy_object(struct atto_object *o)
{
  if (o->kind == ATTO_OBJECT_KIND_LIST) {
    atto_destroy_object(o->container.list.car);
    atto_destroy_object(o->container.list.cdr);
  }

  free(o);
}

void atto_destroy_state(struct atto_state *a)
{
  uint32_t i;
  struct atto_environment_object *current = a->global_environment->head;

  for (i = 0; i < a->number_of_symbols; i++) {
    free(a->symbol_names[i]);
  }

  while (current) {
    struct atto_environment_object *temp = current->next;
    free(current->name);
    free(current);
    current = temp;
  }

  free(a->symbol_names);
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

void atto_add_to_environment(struct atto_environment *env, char *name, uint8_t kind, size_t offset)
{
  char *temp = (char *)malloc(sizeof(char) * (strlen(name) + 1));
  assert(temp != NULL);
  strcpy(temp, name);

  struct atto_environment_object *eo = (struct atto_environment_object *)malloc(sizeof(struct atto_environment_object));
  assert(eo != NULL);

  eo->name = temp;
  eo->kind = kind;
  eo->offset = offset;
  eo->next = env->head;
  env->head = eo;
}

struct atto_environment_object *atto_find_in_environment(struct atto_environment *env, char *name)
{
  struct atto_environment_object *current = env->head;

  while (current) {
    if (strcmp(current->name, name) == 0) {
      return current;
    }

    current = current->next;
  }

  if (env->parent == NULL) {
    return NULL;
  }

  return atto_find_in_environment(env->parent, name);
}

struct atto_object *atto_get_object(struct atto_state *a, struct atto_environment_object *eo)
{
  if (eo->kind == ATTO_ENVIRONMENT_OBJECT_KIND_GLOBAL) {
    return a->vm_state->data_stack[eo->offset];
  }

  if (eo->kind == ATTO_ENVIRONMENT_OBJECT_KIND_LOCAL) {
    return a->vm_state->data_stack[a->vm_state->call_stack[a->vm_state->call_stack_size - 1].stack_offset_at_entrypoint + eo->offset];
  }

  if (eo->kind == ATTO_ENVIRONMENT_OBJECT_KIND_ARGUMENT) {
    return a->vm_state->data_stack[a->vm_state->call_stack[a->vm_state->call_stack_size - 1].stack_offset_at_entrypoint - eo->offset];
  }

  return NULL;
}

