
/*
 *  vm.h
 *  part of Atto :: https://github.com/deveah/atto
 */

#include <stdlib.h>
#include <stdint.h>

#pragma once

/*  flow control */
#define ATTO_VM_OP_NOP    0x00
#define ATTO_VM_OP_CALL   0x01
#define ATTO_VM_OP_RET    0x02
#define ATTO_VM_OP_B      0x03
#define ATTO_VM_OP_BT     0x04
#define ATTO_VM_OP_BF     0x05
#define ATTO_VM_OP_CLOSE  0x06
#define ATTO_VM_OP_STOP   0x07

/*  arithmetic operations */
#define ATTO_VM_OP_ADD    0x10
#define ATTO_VM_OP_SUB    0x11
#define ATTO_VM_OP_MUL    0x12
#define ATTO_VM_OP_DIV    0x13
#define ATTO_VM_OP_ISEQ   0x18
#define ATTO_VM_OP_ISLT   0x19
#define ATTO_VM_OP_ISLET  0x1a
#define ATTO_VM_OP_ISGT   0x1b
#define ATTO_VM_OP_ISGET  0x1c

/*  boolean/symbolic operations */
#define ATTO_VM_OP_ISSEQ  0x20
#define ATTO_VM_OP_NOT    0x21
#define ATTO_VM_OP_OR     0x22
#define ATTO_VM_OP_AND    0x23
#define ATTO_VM_OP_ISNULL 0x24

/*  list operations */
#define ATTO_VM_OP_CAR    0x30
#define ATTO_VM_OP_CDR    0x31
#define ATTO_VM_OP_CONS   0x32

/*  stack operations */
#define ATTO_VM_OP_PUSHN  0x40
#define ATTO_VM_OP_PUSHS  0x41
#define ATTO_VM_OP_PUSHL  0x42
#define ATTO_VM_OP_PUSHZ  0x43

#define ATTO_VM_OP_DUP    0x48
#define ATTO_VM_OP_DROP   0x49
#define ATTO_VM_OP_SWAP   0x4a

#define ATTO_VM_OP_GETGL  0x50
#define ATTO_VM_OP_GETLC  0x51
#define ATTO_VM_OP_GETAG  0x52

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
      struct atto_object *car;
      struct atto_object *cdr;
    } list;
    size_t instruction_stream_index;
  } container;
};

struct atto_instruction_stream {
  size_t length;
  size_t allocated_length;
  uint8_t *stream;
};

struct atto_vm_call_stack_entry {
  size_t instruction_stream_index;
  size_t instruction_offset;
  size_t stack_offset_at_entrypoint;
};

struct atto_vm_state {
  #define ATTO_VM_MAX_DATA_STACK_SIZE (size_t)256
  struct atto_object **data_stack;
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
void evaluate_thunk(struct atto_vm_state *vm, struct atto_object *o);
void atto_run_instruction_stream(struct atto_vm_state *vm, size_t index);

