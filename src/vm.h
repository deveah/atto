
/*
 *  vm.h
 *  part of Atto :: https://github.com/deveah/atto
 */

#include <stdlib.h>
#include <stdint.h>

#include "ops.h"

#pragma once

struct atto_object {
  #define ATTO_OBJECT_KIND_NULL   0
  #define ATTO_OBJECT_KIND_NUMBER 1
  #define ATTO_OBJECT_KIND_SYMBOL 2
  #define ATTO_OBJECT_KIND_LIST   3
  #define ATTO_OBJECT_KIND_LAMBDA 4
  #define ATTO_OBJECT_KIND_THUNK  5
  uint8_t kind;

  union {
    double number;
    uint64_t symbol;
    struct {
      size_t car;
      size_t cdr;
    } list;
    size_t instruction_stream_index;
  } container;
};

struct atto_instruction {
  uint8_t opcode;

  union {
    double number;
    uint64_t symbol;
    size_t offset;
  } container;
};

struct atto_instruction_stream {
  size_t length;
  size_t allocated_length;
  struct atto_instruction *stream;
};

struct atto_vm_call_stack_entry {
  size_t instruction_stream_index;
  size_t instruction_offset;
  size_t stack_offset_at_entrypoint;
};

struct atto_vm_state {
  #define ATTO_VM_MAX_DATA_STACK_SIZE (size_t)256
  size_t *data_stack;
  size_t data_stack_size;

  #define ATTO_VM_MAX_HEAP_OBJECTS (size_t)1024
  struct atto_object *heap;
  size_t heap_size;

  #define ATTO_VM_MAX_CALL_STACK_SIZE (size_t)256
  struct atto_vm_call_stack_entry *call_stack;
  size_t call_stack_size;

  #define ATTO_VM_MIN_NUMBER_OF_INSTRUCTION_STREAMS (size_t)64
  struct atto_instruction_stream **instruction_streams;
  size_t number_of_instruction_streams;
  size_t number_of_allocated_instruction_streams;

  size_t current_instruction_stream_index;
  size_t current_instruction_offset;

  #define ATTO_VM_FLAG_RUNNING (1<<0)
  #define ATTO_VM_FLAG_VERBOSE (1<<1)
  uint8_t flags;
};

struct atto_vm_state *atto_allocate_vm_state(void);
void atto_destroy_vm_state(struct atto_vm_state *vm);
void atto_vm_perform_step(struct atto_vm_state *vm);
void atto_run_vm(struct atto_vm_state *vm);
void pretty_print_stack(struct atto_vm_state *vm);
void pretty_print_heap_usage(struct atto_vm_state *vm);
void evaluate_thunk(struct atto_vm_state *vm, size_t index);
void atto_run_instruction_stream(struct atto_vm_state *vm, size_t index);

