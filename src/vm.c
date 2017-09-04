
/*
 *  vm.c
 *  part of Atto :: https://github.com/deveah/atto
 */

#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "compiler.h"
#include "vm.h"


struct atto_vm_state *atto_allocate_vm_state(void)
{
  struct atto_vm_state *vm = (struct atto_vm_state *)malloc(sizeof(struct atto_vm_state));
  assert(vm != NULL);

  vm->data_stack_size = 0;
  vm->data_stack = (struct atto_object **)malloc(sizeof(struct atto_object *) * ATTO_VM_MAX_DATA_STACK_SIZE);
  assert(vm->data_stack != NULL);

  vm->heap_size = 0;
  vm->heap = (struct atto_object *)malloc(sizeof(struct atto_object) * ATTO_VM_MAX_HEAP_OBJECTS);
  assert(vm->heap != NULL);

  vm->call_stack_size = 0;
  vm->call_stack = (struct atto_vm_call_stack_entry *)malloc(sizeof(struct atto_vm_call_stack_entry) * ATTO_VM_MAX_CALL_STACK_SIZE);
  assert(vm->call_stack != NULL);

  vm->number_of_instruction_streams = 0;
  vm->number_of_allocated_instruction_streams = ATTO_VM_MIN_NUMBER_OF_INSTRUCTION_STREAMS;
  vm->instruction_streams = (struct atto_instruction_stream **)malloc(sizeof(struct atto_instruction_stream *) * ATTO_VM_MIN_NUMBER_OF_INSTRUCTION_STREAMS);

  vm->current_instruction_stream_index = 0;
  vm->current_instruction_stream_index = 0;

  vm->flags = 0x00;

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

  switch (opcode) {
  
  case ATTO_VM_OP_NOP: {
    if (vm->flags & ATTO_VM_FLAG_VERBOSE) {
      printf("vm: %04lu nop\n", vm->current_instruction_offset);
    }

    vm->current_instruction_offset += 1;
    break;
  }

  case ATTO_VM_OP_CALL: {
    size_t target_instruction_stream;

    if (vm->data_stack[vm->data_stack_size - 1]->kind != ATTO_OBJECT_KIND_LAMBDA) {
      printf("vm: fatal: attempting to call non-lambda object\n");
      vm->flags &= ~(ATTO_VM_FLAG_RUNNING);
      return;
    }

    vm->data_stack_size--;

    target_instruction_stream = vm->data_stack[vm->data_stack_size]->container.instruction_stream_index;

    if (vm->flags & ATTO_VM_FLAG_VERBOSE) {
      printf("vm: %04lu call\n", vm->current_instruction_offset);
      pretty_print_instruction_stream(vm->instruction_streams[target_instruction_stream]);
    }

    vm->call_stack[vm->call_stack_size].instruction_stream_index = vm->current_instruction_stream_index;
    vm->call_stack[vm->call_stack_size].instruction_offset = vm->current_instruction_offset + sizeof(uint8_t);
    vm->call_stack[vm->call_stack_size].stack_offset_at_entrypoint = vm->data_stack_size;
    vm->call_stack_size++;

    vm->current_instruction_stream_index = target_instruction_stream;
    vm->current_instruction_offset = 0;
    break;
  }

  case ATTO_VM_OP_RET: {
    if (vm->flags & ATTO_VM_FLAG_VERBOSE) {
      printf("vm: %04lu ret (%lu:%lu)\n", vm->current_instruction_offset,
        vm->call_stack[vm->call_stack_size - 1].instruction_stream_index,
        vm->call_stack[vm->call_stack_size - 1].instruction_offset);
    }

    if (vm->call_stack_size == 0) {
      if (vm->flags & ATTO_VM_FLAG_VERBOSE) {
        printf("vm: finish\n");
      }

      vm->flags &= ~(ATTO_VM_FLAG_RUNNING);
      break;
    }

    vm->call_stack_size--;
    vm->current_instruction_stream_index = vm->call_stack[vm->call_stack_size].instruction_stream_index;
    vm->current_instruction_offset = vm->call_stack[vm->call_stack_size].instruction_offset;
    vm->data_stack_size = vm->call_stack[vm->call_stack_size].stack_offset_at_entrypoint + 1;
    break;
  }

  case ATTO_VM_OP_B: {
    size_t offset;
    memcpy(&offset, current_instruction_stream->stream + vm->current_instruction_offset + 1, sizeof(size_t));

    if (vm->flags & ATTO_VM_FLAG_VERBOSE) {
      printf("vm: %04lu b %lu\n", vm->current_instruction_offset, offset);
    }

    vm->current_instruction_offset = offset;
    break;
  }

  case ATTO_VM_OP_BT: {
    size_t offset;
    memcpy(&offset, current_instruction_stream->stream + vm->current_instruction_offset + 1, sizeof(size_t));

    if (vm->flags & ATTO_VM_FLAG_VERBOSE) {
      printf("vm: %04lu bt %lu\n", vm->current_instruction_offset, offset);
    }

    if (vm->data_stack[vm->data_stack_size - 1]->kind != ATTO_OBJECT_KIND_SYMBOL) {
      printf("vm: fatal: attempting to conditionally branch, but no symbol is present.\n");
      vm->flags &= ~(ATTO_VM_FLAG_RUNNING);
      break;
    }

    vm->data_stack_size--;
    if (vm->data_stack[vm->data_stack_size]->container.symbol == 1) {
      vm->current_instruction_offset = offset;
      break;
    }

    vm->current_instruction_offset += sizeof(uint8_t) + sizeof(size_t);
    break;
  }

  case ATTO_VM_OP_BF: {
    size_t offset;
    memcpy(&offset, current_instruction_stream->stream + vm->current_instruction_offset + 1, sizeof(size_t));

    if (vm->flags & ATTO_VM_FLAG_VERBOSE) {
      printf("vm: %04lu bt %lu\n", vm->current_instruction_offset, offset);
    }

    if (vm->data_stack[vm->data_stack_size - 1]->kind != ATTO_OBJECT_KIND_SYMBOL) {
      printf("vm: fatal: attempting to conditionally branch, but no symbol is present.\n");
      vm->flags &= ~(ATTO_VM_FLAG_RUNNING);
      break;
    }

    vm->data_stack_size--;
    if (vm->data_stack[vm->data_stack_size]->container.symbol == 0) {
      vm->current_instruction_offset = offset;
      break;
    }

    vm->current_instruction_offset += sizeof(uint8_t) + sizeof(size_t);
    break;
  }

  case ATTO_VM_OP_CLOSE: {
    size_t count;
    memcpy(&count, current_instruction_stream->stream + vm->current_instruction_offset + 1, sizeof(size_t));

    if (vm->flags & ATTO_VM_FLAG_VERBOSE) {
      printf("vm: %04lu close %lu\n", vm->current_instruction_offset, count);
    }

    vm->data_stack[vm->data_stack_size - count - 1] = vm->data_stack[vm->data_stack_size - 1];
    vm->data_stack_size = vm->data_stack_size - count;

    vm->current_instruction_offset += 1 + sizeof(size_t);
    break;
  }

  case ATTO_VM_OP_STOP: {
    if (vm->flags & ATTO_VM_FLAG_VERBOSE) {
      printf("vm: %04lu stop\n", vm->current_instruction_offset);
    }

    vm->flags &= ~(ATTO_VM_FLAG_RUNNING);
    break;
  }

  case ATTO_VM_OP_ADD: {
    struct atto_object *a = vm->data_stack[vm->data_stack_size - 1];
    struct atto_object *b = vm->data_stack[vm->data_stack_size - 2];
    struct atto_object *c = &vm->heap[vm->heap_size++];

    if (vm->flags & ATTO_VM_FLAG_VERBOSE) {
      printf("vm: %04lu add\n", vm->current_instruction_offset);
    }

    if (a->kind == ATTO_OBJECT_KIND_THUNK) {
      evaluate_thunk(vm, a);
    }

    if (b->kind == ATTO_OBJECT_KIND_THUNK) {
      evaluate_thunk(vm, b);
    }

    if ((a->kind != ATTO_OBJECT_KIND_NUMBER) ||
        (b->kind != ATTO_OBJECT_KIND_NUMBER)) {
      printf("vm: fatal: attempting to perform `add' on non-numeric arguments\n");
      vm->flags &= ~(ATTO_VM_FLAG_RUNNING);
      return;
    }

    vm->data_stack_size -= 1;
    c->kind = ATTO_OBJECT_KIND_NUMBER;
    c->container.number = a->container.number + b->container.number;
    vm->data_stack[vm->data_stack_size - 1] = c;

    vm->current_instruction_offset += 1;
    break;
  }

  case ATTO_VM_OP_SUB: {
    struct atto_object *a = vm->data_stack[vm->data_stack_size - 1];
    struct atto_object *b = vm->data_stack[vm->data_stack_size - 2];
    struct atto_object *c = &vm->heap[vm->heap_size++];

    if (vm->flags & ATTO_VM_FLAG_VERBOSE) {
      printf("vm: %04lu sub\n", vm->current_instruction_offset);
    }

    if (a->kind == ATTO_OBJECT_KIND_THUNK) {
      evaluate_thunk(vm, a);
    }

    if (b->kind == ATTO_OBJECT_KIND_THUNK) {
      evaluate_thunk(vm, b);
    }

    if ((a->kind != ATTO_OBJECT_KIND_NUMBER) ||
        (b->kind != ATTO_OBJECT_KIND_NUMBER)) {
      printf("vm: fatal: attempting to perform `add' on non-numeric arguments\n");
      vm->flags &= ~(ATTO_VM_FLAG_RUNNING);
      return;
    }

    vm->data_stack_size -= 1;
    c->kind = ATTO_OBJECT_KIND_NUMBER;
    c->container.number = a->container.number - b->container.number;
    vm->data_stack[vm->data_stack_size - 1] = c;

    vm->current_instruction_offset += 1;
    break;
  }

  case ATTO_VM_OP_MUL: {
    struct atto_object *a = vm->data_stack[vm->data_stack_size - 1];
    struct atto_object *b = vm->data_stack[vm->data_stack_size - 2];
    struct atto_object *c = &vm->heap[vm->heap_size++];

    if (vm->flags & ATTO_VM_FLAG_VERBOSE) {
      printf("vm: %04lu mul\n", vm->current_instruction_offset);
    }

    if (a->kind == ATTO_OBJECT_KIND_THUNK) {
      evaluate_thunk(vm, a);
    }

    if (b->kind == ATTO_OBJECT_KIND_THUNK) {
      evaluate_thunk(vm, b);
    }

    if ((a->kind != ATTO_OBJECT_KIND_NUMBER) ||
        (b->kind != ATTO_OBJECT_KIND_NUMBER)) {
      printf("vm: fatal: attempting to perform `add' on non-numeric arguments\n");
      vm->flags &= ~(ATTO_VM_FLAG_RUNNING);
      return;
    }

    vm->data_stack_size -= 1;
    c->kind = ATTO_OBJECT_KIND_NUMBER;
    c->container.number = a->container.number * b->container.number;
    vm->data_stack[vm->data_stack_size - 1] = c;

    vm->current_instruction_offset += 1;
    break;
  }

  case ATTO_VM_OP_DIV: {
    struct atto_object *a = vm->data_stack[vm->data_stack_size - 1];
    struct atto_object *b = vm->data_stack[vm->data_stack_size - 2];
    struct atto_object *c = &vm->heap[vm->heap_size++];

    if (vm->flags & ATTO_VM_FLAG_VERBOSE) {
      printf("vm: %04lu div\n", vm->current_instruction_offset);
    }

    if (a->kind == ATTO_OBJECT_KIND_THUNK) {
      evaluate_thunk(vm, a);
    }

    if (b->kind == ATTO_OBJECT_KIND_THUNK) {
      evaluate_thunk(vm, b);
    }

    if ((a->kind != ATTO_OBJECT_KIND_NUMBER) ||
        (b->kind != ATTO_OBJECT_KIND_NUMBER)) {
      printf("vm: fatal: attempting to perform `add' on non-numeric arguments\n");
      vm->flags &= ~(ATTO_VM_FLAG_RUNNING);
      return;
    }

    vm->data_stack_size -= 1;
    c->kind = ATTO_OBJECT_KIND_NUMBER;
    c->container.number = a->container.number / b->container.number;
    vm->data_stack[vm->data_stack_size - 1] = c;

    vm->current_instruction_offset += 1;
    break;
  }

  case ATTO_VM_OP_ISEQ: {
    struct atto_object *a = vm->data_stack[vm->data_stack_size - 1];
    struct atto_object *b = vm->data_stack[vm->data_stack_size - 2];
    struct atto_object *c = &vm->heap[vm->heap_size++];

    if (vm->flags & ATTO_VM_FLAG_VERBOSE) {
      printf("vm: %04lu iseq\n", vm->current_instruction_offset);
    }

    if ((a->kind != ATTO_OBJECT_KIND_NUMBER) ||
        (b->kind != ATTO_OBJECT_KIND_NUMBER)) {
      printf("vm: fatal: attempting to perform `iseq' on non-numeric arguments\n");
      vm->flags &= ~(ATTO_VM_FLAG_RUNNING);
      return;
    }

    vm->data_stack_size -= 1;
    c->kind = ATTO_OBJECT_KIND_SYMBOL;
    c->container.symbol = (a->container.number == b->container.number);
    vm->data_stack[vm->data_stack_size - 1] = c;

    vm->current_instruction_offset += 1;
    break;
  }

  case ATTO_VM_OP_ISLT: {
    struct atto_object *a = vm->data_stack[vm->data_stack_size - 1];
    struct atto_object *b = vm->data_stack[vm->data_stack_size - 2];
    struct atto_object *c = &vm->heap[vm->heap_size++];

    if (vm->flags & ATTO_VM_FLAG_VERBOSE) {
      printf("vm: %04lu islt\n", vm->current_instruction_offset);
    }

    if ((a->kind != ATTO_OBJECT_KIND_NUMBER) ||
        (b->kind != ATTO_OBJECT_KIND_NUMBER)) {
      printf("fatal: attempting to perform `islt' on non-numeric arguments\n");
      vm->flags &= ~(ATTO_VM_FLAG_RUNNING);
      return;
    }

    vm->data_stack_size -= 1;
    c->kind = ATTO_OBJECT_KIND_SYMBOL;
    c->container.symbol = (a->container.number < b->container.number);
    vm->data_stack[vm->data_stack_size - 1] = c;

    vm->current_instruction_offset += 1;
    break;
  }

  case ATTO_VM_OP_ISLET: {
    struct atto_object *a = vm->data_stack[vm->data_stack_size - 1];
    struct atto_object *b = vm->data_stack[vm->data_stack_size - 2];
    struct atto_object *c = &vm->heap[vm->heap_size++];

    if (vm->flags & ATTO_VM_FLAG_VERBOSE) {
      printf("vm: islet\n");
      printf("vm: %04lu islet\n", vm->current_instruction_offset);
    }

    if ((a->kind != ATTO_OBJECT_KIND_NUMBER) ||
        (b->kind != ATTO_OBJECT_KIND_NUMBER)) {
      printf("fatal: attempting to perform `islet' on non-numeric arguments\n");
      vm->flags &= ~(ATTO_VM_FLAG_RUNNING);
      return;
    }

    vm->data_stack_size -= 1;
    c->kind = ATTO_OBJECT_KIND_SYMBOL;
    c->container.symbol = (a->container.number <= b->container.number);
    vm->data_stack[vm->data_stack_size - 1] = c;

    vm->current_instruction_offset += 1;
    break;
  }

  case ATTO_VM_OP_ISGT: {
    struct atto_object *a = vm->data_stack[vm->data_stack_size - 1];
    struct atto_object *b = vm->data_stack[vm->data_stack_size - 2];
    struct atto_object *c = &vm->heap[vm->heap_size++];

    if (vm->flags & ATTO_VM_FLAG_VERBOSE) {
      printf("vm: %04lu isgt\n", vm->current_instruction_offset);
    }

    if ((a->kind != ATTO_OBJECT_KIND_NUMBER) ||
        (b->kind != ATTO_OBJECT_KIND_NUMBER)) {
      printf("fatal: attempting to perform `isgt' on non-numeric arguments\n");
      vm->flags &= ~(ATTO_VM_FLAG_RUNNING);
      return;
    }

    vm->data_stack_size -= 1;
    c->kind = ATTO_OBJECT_KIND_SYMBOL;
    c->container.symbol = (a->container.number > b->container.number);
    vm->data_stack[vm->data_stack_size - 1] = c;

    vm->current_instruction_offset += 1;
    break;
  }

  case ATTO_VM_OP_ISGET: {
    struct atto_object *a = vm->data_stack[vm->data_stack_size - 1];
    struct atto_object *b = vm->data_stack[vm->data_stack_size - 2];
    struct atto_object *c = &vm->heap[vm->heap_size++];

    if (vm->flags & ATTO_VM_FLAG_VERBOSE) {
      printf("vm: %04lu isget\n", vm->current_instruction_offset);
    }

    if ((a->kind != ATTO_OBJECT_KIND_NUMBER) ||
        (b->kind != ATTO_OBJECT_KIND_NUMBER)) {
      printf("fatal: attempting to perform `isget' on non-numeric arguments\n");
      vm->flags &= ~(ATTO_VM_FLAG_RUNNING);
      return;
    }

    vm->data_stack_size -= 1;
    c->kind = ATTO_OBJECT_KIND_SYMBOL;
    c->container.symbol = (a->container.number >= b->container.number);
    vm->data_stack[vm->data_stack_size - 1] = c;

    vm->current_instruction_offset += 1;
    break;
  }

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

    if (vm->flags & ATTO_VM_FLAG_VERBOSE) {
      printf("vm: %04lu push_number %lf\n", vm->current_instruction_offset, number);
    }

    vm->heap[vm->heap_size].kind = ATTO_OBJECT_KIND_NUMBER;
    vm->heap[vm->heap_size].container.number = number;
    vm->heap_size++;

    vm->data_stack[vm->data_stack_size] = &vm->heap[vm->heap_size - 1];
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

  case ATTO_VM_OP_GETGL: {
    size_t index;
    memcpy(&index, current_instruction_stream->stream + vm->current_instruction_offset + 1, sizeof(size_t));

    if (vm->flags & ATTO_VM_FLAG_VERBOSE) {
      printf("vm: %04lu getgl %lu\n", vm->current_instruction_offset, index);
    }

    vm->data_stack[vm->data_stack_size] = vm->data_stack[index];
    vm->data_stack_size++;

    vm->current_instruction_offset += 1 + sizeof(size_t);
    break;
  }

  case ATTO_VM_OP_GETLC: {
    size_t index;
    memcpy(&index, current_instruction_stream->stream + vm->current_instruction_offset + 1, sizeof(size_t));

    if (vm->flags & ATTO_VM_FLAG_VERBOSE) {
      printf("vm: getlc %lu\n", index);
      printf("vm: %04lu getlc %lu\n", vm->current_instruction_offset, index);
    }

    vm->data_stack[vm->data_stack_size] = vm->data_stack[vm->call_stack[vm->call_stack_size - 1].stack_offset_at_entrypoint + index];
    vm->data_stack_size++;

    vm->current_instruction_offset += 1 + sizeof(size_t);
    break;
  }

  case ATTO_VM_OP_GETAG: {
    size_t index;
    memcpy(&index, current_instruction_stream->stream + vm->current_instruction_offset + 1, sizeof(size_t));

    if (vm->flags & ATTO_VM_FLAG_VERBOSE) {
      printf("vm: %04lu getag %lu\n", vm->current_instruction_offset, index);
    }

    vm->data_stack[vm->data_stack_size] = vm->data_stack[vm->call_stack[vm->call_stack_size - 1].stack_offset_at_entrypoint - index - 1];
    vm->data_stack_size++;

    vm->current_instruction_offset += 1 + sizeof(size_t);
    break;
  }

  default:
    printf("vm: fatal: unknown opcode (0x%02x)\n", opcode);
    exit(1);
  }

  pretty_print_stack(vm);
}

void atto_run_vm(struct atto_vm_state *vm)
{
  vm->flags |= ATTO_VM_FLAG_RUNNING;

  if (vm->flags & ATTO_VM_FLAG_VERBOSE) {
    printf("vm: run is=%lu, o=%lu\n", vm->current_instruction_stream_index, vm->current_instruction_offset);
  }

  while (vm->flags & ATTO_VM_FLAG_RUNNING) {
    atto_vm_perform_step(vm);

    if (vm->current_instruction_offset >= vm->instruction_streams[vm->current_instruction_stream_index]->length) {
      vm->flags &= ~(ATTO_VM_FLAG_RUNNING);

      if (vm->flags & ATTO_VM_FLAG_VERBOSE) {
        printf("vm: reached end of instruction stream\n");
      }
    }
  }
}

void pretty_print_stack(struct atto_vm_state *vm)
{
  size_t i;

  printf("stack: ");

  for (i = 0; i < vm->data_stack_size; i++) {
    
    if ((vm->call_stack_size > 0) &&
        (i == vm->call_stack[vm->call_stack_size - 1].stack_offset_at_entrypoint)) {
      printf("| ");
    }

    switch (vm->data_stack[i]->kind) {
    
    case ATTO_OBJECT_KIND_NULL:
      printf("() ");
      break;

    case ATTO_OBJECT_KIND_NUMBER:
      printf("num(%lf) ", vm->data_stack[i]->container.number);
      break;

    case ATTO_OBJECT_KIND_SYMBOL:
      printf("sym(%lu) ", vm->data_stack[i]->container.symbol);
      break;

    case ATTO_OBJECT_KIND_LIST:
      printf("list() ");
      break;

    case ATTO_OBJECT_KIND_LAMBDA:
      printf("lambda() ");
      break;

    case ATTO_OBJECT_KIND_THUNK:
      printf("thunk(%lu) ", vm->data_stack[i]->container.instruction_stream_index);
      break;

    default:
      printf("bug ");
    }
  }

  printf("\n");
}

void pretty_print_heap_usage(struct atto_vm_state *vm)
{
  printf("heap: %lu/%lu objects\n", vm->heap_size, ATTO_VM_MAX_HEAP_OBJECTS);
}

void evaluate_thunk(struct atto_vm_state *vm, struct atto_object *o)
{
  if (o->kind != ATTO_OBJECT_KIND_THUNK) {
    return;
  }

  atto_run_instruction_stream(vm, o->container.instruction_stream_index);

  /*  TODO: free linked instruction stream */

  o->kind = vm->data_stack[vm->data_stack_size - 1]->kind;
  o->container = vm->data_stack[vm->data_stack_size - 1]->container;
}

void atto_run_instruction_stream(struct atto_vm_state *vm, size_t index)
{
  if (vm->flags & ATTO_VM_FLAG_VERBOSE) {
    pretty_print_instruction_stream(vm->instruction_streams[index]);
  }

  vm->call_stack[vm->call_stack_size].instruction_stream_index = vm->current_instruction_stream_index;
  vm->call_stack[vm->call_stack_size].instruction_offset = vm->current_instruction_offset;
  vm->call_stack[vm->call_stack_size].stack_offset_at_entrypoint = vm->data_stack_size;
  vm->call_stack_size++;

  vm->current_instruction_stream_index = index;
  vm->current_instruction_offset = 0;
  atto_run_vm(vm);
}

