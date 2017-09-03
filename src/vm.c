
/*
 *  vm.c
 *  part of Atto :: https://github.com/deveah/atto
 */

#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "vm.h"


struct atto_vm_state *atto_allocate_vm_state(void)
{
  struct atto_vm_state *vm = (struct atto_vm_state *)malloc(sizeof(struct atto_vm_state));
  assert(vm != NULL);

  vm->data_stack_size = 0;
  vm->data_stack = (struct atto_object *)malloc(sizeof(struct atto_object) * ATTO_VM_MAX_DATA_STACK_SIZE);
  assert(vm->data_stack != NULL);

  vm->call_stack_size = 0;
  vm->call_stack = (struct atto_vm_call_stack_entry *)malloc(sizeof(struct atto_vm_call_stack_entry) * ATTO_VM_MAX_CALL_STACK_SIZE);
  assert(vm->call_stack != NULL);

  vm->number_of_instruction_streams = 0;
  vm->number_of_allocated_instruction_streams = ATTO_VM_MIN_NUMBER_OF_INSTRUCTION_STREAMS;
  vm->instruction_streams = (struct atto_instruction_stream **)malloc(sizeof(struct atto_instruction_stream *) * ATTO_VM_MIN_NUMBER_OF_INSTRUCTION_STREAMS);

  vm->current_instruction_stream_index = 0;
  vm->current_instruction_stream_index = 0;

  return vm;
}

void atto_destroy_vm_state(struct atto_vm_state *vm)
{
  free(vm->data_stack);
  free(vm->call_stack);
  free(vm->instruction_streams);
  free(vm);
}

void atto_vm_perform_step(struct atto_vm_state *vm)
{
  struct atto_instruction_stream *current_instruction_stream = vm->instruction_streams[vm->current_instruction_stream_index];
  uint8_t opcode = current_instruction_stream->stream[vm->current_instruction_offset];

  printf("vm: op=0x%02x\n", opcode);

  switch (opcode) {
  
  case ATTO_VM_OP_NOP: {
    vm->current_instruction_offset += 1;
    break;
  }

  case ATTO_VM_OP_CALL: {
    size_t target_instruction_stream;

    if (vm->data_stack[vm->data_stack_size - 1].kind != ATTO_OBJECT_KIND_LAMBDA) {
      printf("fatal: attempting to call non-lambda object\n");
      return;
    }

    target_instruction_stream = vm->data_stack[vm->data_stack_size - 1].container.instruction_stream_index;

    vm->call_stack[vm->call_stack_size].instruction_stream_index = vm->current_instruction_stream_index;
    vm->call_stack[vm->call_stack_size].instruction_offset = vm->current_instruction_offset + 1 + sizeof(size_t);
    vm->call_stack[vm->call_stack_size].stack_offset_at_entrypoint = vm->data_stack_size;
    vm->call_stack_size++;

    vm->current_instruction_stream_index = target_instruction_stream;
    vm->current_instruction_offset = 0;
    break;
  }

  case ATTO_VM_OP_RET: {
    vm->call_stack_size -= 1;
    vm->current_instruction_stream_index = vm->call_stack[vm->call_stack_size].instruction_stream_index;
    vm->current_instruction_offset = vm->call_stack[vm->call_stack_size].instruction_offset;
    break;
  }

  case ATTO_VM_OP_B:
    break;

  case ATTO_VM_OP_BT:
    break;

  case ATTO_VM_OP_BF:
    break;

  case ATTO_VM_OP_ADD: {
    struct atto_object *a = &vm->data_stack[vm->data_stack_size - 1];
    struct atto_object *b = &vm->data_stack[vm->data_stack_size - 2];

    if ((a->kind != ATTO_OBJECT_KIND_NUMBER) ||
        (b->kind != ATTO_OBJECT_KIND_NUMBER)) {
      printf("fatal: attempting to perform `add' on non-numeric arguments\n");
      return;
    }

    vm->data_stack_size -= 1;
    b->container.number += a->container.number;

    vm->current_instruction_offset += 1;
    break;
  }

  case ATTO_VM_OP_SUB:
    break;

  case ATTO_VM_OP_MUL:
    break;

  case ATTO_VM_OP_DIV:
    break;

  case ATTO_VM_OP_ISEQ:
    break;

  case ATTO_VM_OP_ISLT:
    break;

  case ATTO_VM_OP_ISLET:
    break;

  case ATTO_VM_OP_ISGT:
    break;

  case ATTO_VM_OP_ISGET:
    break;

  case ATTO_VM_OP_ISSEQ:
    break;

  case ATTO_VM_OP_NOT:
    break;

  case ATTO_VM_OP_OR:
    break;

  case ATTO_VM_OP_AND:
    break;

  case ATTO_VM_OP_CAR:
    break;

  case ATTO_VM_OP_CDR:
    break;

  case ATTO_VM_OP_CONS:
    break;

  case ATTO_VM_OP_PUSHN: {
    double number;
    memcpy(&number, current_instruction_stream->stream + vm->current_instruction_offset + 1, sizeof(double));

    vm->data_stack[vm->data_stack_size].kind = ATTO_OBJECT_KIND_NUMBER;
    vm->data_stack[vm->data_stack_size].container.number = number;
    vm->data_stack_size++;

    vm->current_instruction_offset += 1 + sizeof(double);
    break;
  }

  case ATTO_VM_OP_PUSHS:
    break;

  case ATTO_VM_OP_PUSHZ:
    break;

  case ATTO_VM_OP_DUP:
    break;

  case ATTO_VM_OP_DROP:
    break;

  case ATTO_VM_OP_SWAP:
    break;

  case ATTO_VM_OP_GETGL:
    break;

  case ATTO_VM_OP_GETLC:
    break;

  case ATTO_VM_OP_GETAG:
    break;

  default:
    printf("fatal: unknown opcode\n");
  }
}

