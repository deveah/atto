
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
  vm->data_stack = (size_t *)malloc(sizeof(size_t) * ATTO_VM_MAX_DATA_STACK_SIZE);
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
  struct atto_instruction *current_instruction = &current_instruction_stream->stream[vm->current_instruction_offset];

  switch (current_instruction->opcode) {
  
  case ATTO_VM_OP_NOP: {
    if (vm->flags & ATTO_VM_FLAG_VERBOSE) {
      printf("vm: %04lu nop\n", vm->current_instruction_offset);
    }

    vm->current_instruction_offset += 1;
    break;
  }

  case ATTO_VM_OP_CALL: {
    size_t target_instruction_stream;

    struct atto_object *fn = &vm->heap[vm->data_stack[vm->data_stack_size-1]];

    if (fn->kind != ATTO_OBJECT_KIND_LAMBDA) {
      printf("vm: fatal: attempting to call non-lambda object\n");
      vm->flags &= ~(ATTO_VM_FLAG_RUNNING);
      return;
    }

    vm->data_stack_size--;

    target_instruction_stream = fn->container.instruction_stream_index;

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
      size_t destination_function = vm->call_stack[vm->call_stack_size-1].instruction_stream_index,
             destination_offset   = vm->call_stack[vm->call_stack_size-1].instruction_offset;
      printf("vm: %04lu ret (%lu:%lu)\n", vm->current_instruction_offset, destination_function, destination_offset);
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
    if (vm->flags & ATTO_VM_FLAG_VERBOSE) {
      printf("vm: %04lu b %lu\n", vm->current_instruction_offset, current_instruction->container.offset);
    }

    vm->current_instruction_offset = current_instruction->container.offset;
    break;
  }

  case ATTO_VM_OP_BT: {
    if (vm->flags & ATTO_VM_FLAG_VERBOSE) {
      printf("vm: %04lu bt %lu\n", vm->current_instruction_offset, current_instruction->container.offset);
    }

    if (vm->heap[vm->data_stack[vm->data_stack_size-1]].kind != ATTO_OBJECT_KIND_SYMBOL) {
      printf("vm: fatal: attempting to conditionally branch, but no symbol is present.\n");
      vm->flags &= ~(ATTO_VM_FLAG_RUNNING);
      break;
    }

    vm->data_stack_size--;
    if (vm->heap[vm->data_stack[vm->data_stack_size]].container.symbol == 1) {
      vm->current_instruction_offset = current_instruction->container.offset;
      break;
    }

    vm->current_instruction_offset++;
    break;
  }

  case ATTO_VM_OP_BF: {
    if (vm->flags & ATTO_VM_FLAG_VERBOSE) {
      printf("vm: %04lu bt %lu\n", vm->current_instruction_offset, current_instruction->container.offset);
    }

    if (vm->heap[vm->data_stack[vm->data_stack_size-1]].kind != ATTO_OBJECT_KIND_SYMBOL) {
      printf("vm: fatal: attempting to conditionally branch, but no symbol is present.\n");
      vm->flags &= ~(ATTO_VM_FLAG_RUNNING);
      break;
    }

    vm->data_stack_size--;
    if (vm->heap[vm->data_stack[vm->data_stack_size]].container.symbol == 0) {
      vm->current_instruction_offset = current_instruction->container.offset;
      break;
    }

    vm->current_instruction_offset++;
    break;
  }

  case ATTO_VM_OP_CLOSE: {
    if (vm->flags & ATTO_VM_FLAG_VERBOSE) {
      printf("vm: %04lu close %lu\n", vm->current_instruction_offset, current_instruction->container.offset);
    }

    vm->data_stack[vm->data_stack_size - current_instruction->container.offset - 1] = vm->data_stack[vm->data_stack_size - 1];
    vm->data_stack_size = vm->data_stack_size - current_instruction->container.offset;

    vm->current_instruction_offset++;
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
    size_t a = vm->data_stack[vm->data_stack_size-1],
           b = vm->data_stack[vm->data_stack_size-2],
           c = vm->heap_size++;

    if (vm->flags & ATTO_VM_FLAG_VERBOSE) {
      printf("vm: %04lu add\n", vm->current_instruction_offset);
    }

    if (vm->heap[a].kind == ATTO_OBJECT_KIND_THUNK) {
      evaluate_thunk(vm, a);
    }

    if (vm->heap[b].kind == ATTO_OBJECT_KIND_THUNK) {
      evaluate_thunk(vm, b);
    }

    if ((vm->heap[a].kind != ATTO_OBJECT_KIND_NUMBER) ||
        (vm->heap[b].kind != ATTO_OBJECT_KIND_NUMBER)) {
      printf("vm: fatal: attempting to perform `add' on non-numeric arguments\n");
      vm->flags &= ~(ATTO_VM_FLAG_RUNNING);
      return;
    }

    vm->data_stack_size -= 1;
    vm->heap[c].kind = ATTO_OBJECT_KIND_NUMBER;
    vm->heap[c].container.number = vm->heap[a].container.number + vm->heap[b].container.number;
    vm->data_stack[vm->data_stack_size - 1] = c;

    vm->current_instruction_offset++;
    break;
  }

  case ATTO_VM_OP_SUB: {
    size_t a = vm->data_stack[vm->data_stack_size-1],
           b = vm->data_stack[vm->data_stack_size-2],
           c = vm->heap_size++;

    if (vm->flags & ATTO_VM_FLAG_VERBOSE) {
      printf("vm: %04lu sub\n", vm->current_instruction_offset);
    }

    if (vm->heap[a].kind == ATTO_OBJECT_KIND_THUNK) {
      evaluate_thunk(vm, a);
    }

    if (vm->heap[b].kind == ATTO_OBJECT_KIND_THUNK) {
      evaluate_thunk(vm, b);
    }

    if ((vm->heap[a].kind != ATTO_OBJECT_KIND_NUMBER) ||
        (vm->heap[b].kind != ATTO_OBJECT_KIND_NUMBER)) {
      printf("vm: fatal: attempting to perform `sub' on non-numeric arguments\n");
      vm->flags &= ~(ATTO_VM_FLAG_RUNNING);
      return;
    }

    vm->data_stack_size -= 1;
    vm->heap[c].kind = ATTO_OBJECT_KIND_NUMBER;
    vm->heap[c].container.number = vm->heap[a].container.number - vm->heap[b].container.number;
    vm->data_stack[vm->data_stack_size - 1] = c;

    vm->current_instruction_offset++;
    break;
  }

  case ATTO_VM_OP_MUL: {
    size_t a = vm->data_stack[vm->data_stack_size-1],
           b = vm->data_stack[vm->data_stack_size-2],
           c = vm->heap_size++;

    if (vm->flags & ATTO_VM_FLAG_VERBOSE) {
      printf("vm: %04lu mul\n", vm->current_instruction_offset);
    }

    if (vm->heap[a].kind == ATTO_OBJECT_KIND_THUNK) {
      evaluate_thunk(vm, a);
    }

    if (vm->heap[b].kind == ATTO_OBJECT_KIND_THUNK) {
      evaluate_thunk(vm, b);
    }

    if ((vm->heap[a].kind != ATTO_OBJECT_KIND_NUMBER) ||
        (vm->heap[b].kind != ATTO_OBJECT_KIND_NUMBER)) {
      printf("vm: fatal: attempting to perform `mul' on non-numeric arguments\n");
      vm->flags &= ~(ATTO_VM_FLAG_RUNNING);
      return;
    }

    vm->data_stack_size -= 1;
    vm->heap[c].kind = ATTO_OBJECT_KIND_NUMBER;
    vm->heap[c].container.number = vm->heap[a].container.number * vm->heap[b].container.number;
    vm->data_stack[vm->data_stack_size - 1] = c;

    vm->current_instruction_offset++;
    break;
  }

  case ATTO_VM_OP_DIV: {
    size_t a = vm->data_stack[vm->data_stack_size-1],
           b = vm->data_stack[vm->data_stack_size-2],
           c = vm->heap_size++;

    if (vm->flags & ATTO_VM_FLAG_VERBOSE) {
      printf("vm: %04lu mul\n", vm->current_instruction_offset);
    }

    if (vm->heap[a].kind == ATTO_OBJECT_KIND_THUNK) {
      evaluate_thunk(vm, a);
    }

    if (vm->heap[b].kind == ATTO_OBJECT_KIND_THUNK) {
      evaluate_thunk(vm, b);
    }

    if ((vm->heap[a].kind != ATTO_OBJECT_KIND_NUMBER) ||
        (vm->heap[b].kind != ATTO_OBJECT_KIND_NUMBER)) {
      printf("vm: fatal: attempting to perform `div' on non-numeric arguments\n");
      vm->flags &= ~(ATTO_VM_FLAG_RUNNING);
      return;
    }

    vm->data_stack_size -= 1;
    vm->heap[c].kind = ATTO_OBJECT_KIND_NUMBER;
    vm->heap[c].container.number = vm->heap[a].container.number * vm->heap[b].container.number;
    vm->data_stack[vm->data_stack_size - 1] = c;

    vm->current_instruction_offset++;
    break;
  }

  case ATTO_VM_OP_ISEQ: {
    size_t a = vm->data_stack[vm->data_stack_size-1],
           b = vm->data_stack[vm->data_stack_size-2],
           c = vm->heap_size++;

    if (vm->flags & ATTO_VM_FLAG_VERBOSE) {
      printf("vm: %04lu iseq\n", vm->current_instruction_offset);
    }

    if (vm->heap[a].kind == ATTO_OBJECT_KIND_THUNK) {
      evaluate_thunk(vm, a);
    }

    if (vm->heap[b].kind == ATTO_OBJECT_KIND_THUNK) {
      evaluate_thunk(vm, b);
    }

    if ((vm->heap[a].kind != ATTO_OBJECT_KIND_NUMBER) ||
        (vm->heap[b].kind != ATTO_OBJECT_KIND_NUMBER)) {
      printf("vm: fatal: attempting to perform `iseq' on non-numeric arguments\n");
      vm->flags &= ~(ATTO_VM_FLAG_RUNNING);
      return;
    }

    vm->data_stack_size -= 1;
    vm->heap[c].kind = ATTO_OBJECT_KIND_SYMBOL;
    vm->heap[c].container.number = (vm->heap[a].container.number == vm->heap[b].container.number);
    vm->data_stack[vm->data_stack_size - 1] = c;

    vm->current_instruction_offset++;
    break;
  }

  case ATTO_VM_OP_ISLT: {
    size_t a = vm->data_stack[vm->data_stack_size-1],
           b = vm->data_stack[vm->data_stack_size-2],
           c = vm->heap_size++;

    if (vm->flags & ATTO_VM_FLAG_VERBOSE) {
      printf("vm: %04lu islt\n", vm->current_instruction_offset);
    }

    if (vm->heap[a].kind == ATTO_OBJECT_KIND_THUNK) {
      evaluate_thunk(vm, a);
    }

    if (vm->heap[b].kind == ATTO_OBJECT_KIND_THUNK) {
      evaluate_thunk(vm, b);
    }

    if ((vm->heap[a].kind != ATTO_OBJECT_KIND_NUMBER) ||
        (vm->heap[b].kind != ATTO_OBJECT_KIND_NUMBER)) {
      printf("vm: fatal: attempting to perform `islt' on non-numeric arguments\n");
      vm->flags &= ~(ATTO_VM_FLAG_RUNNING);
      return;
    }

    vm->data_stack_size -= 1;
    vm->heap[c].kind = ATTO_OBJECT_KIND_SYMBOL;
    vm->heap[c].container.number = (vm->heap[a].container.number < vm->heap[b].container.number);
    vm->data_stack[vm->data_stack_size - 1] = c;

    vm->current_instruction_offset++;
    break;
  }

  case ATTO_VM_OP_ISLET: {
    size_t a = vm->data_stack[vm->data_stack_size-1],
           b = vm->data_stack[vm->data_stack_size-2],
           c = vm->heap_size++;

    if (vm->flags & ATTO_VM_FLAG_VERBOSE) {
      printf("vm: %04lu islt\n", vm->current_instruction_offset);
    }

    if (vm->heap[a].kind == ATTO_OBJECT_KIND_THUNK) {
      evaluate_thunk(vm, a);
    }

    if (vm->heap[b].kind == ATTO_OBJECT_KIND_THUNK) {
      evaluate_thunk(vm, b);
    }

    if ((vm->heap[a].kind != ATTO_OBJECT_KIND_NUMBER) ||
        (vm->heap[b].kind != ATTO_OBJECT_KIND_NUMBER)) {
      printf("vm: fatal: attempting to perform `islet' on non-numeric arguments\n");
      vm->flags &= ~(ATTO_VM_FLAG_RUNNING);
      return;
    }

    vm->data_stack_size -= 1;
    vm->heap[c].kind = ATTO_OBJECT_KIND_SYMBOL;
    vm->heap[c].container.number = (vm->heap[a].container.number <= vm->heap[b].container.number);
    vm->data_stack[vm->data_stack_size - 1] = c;

    vm->current_instruction_offset++;
    break;
  }

  case ATTO_VM_OP_ISGT: {
    size_t a = vm->data_stack[vm->data_stack_size-1],
           b = vm->data_stack[vm->data_stack_size-2],
           c = vm->heap_size++;

    if (vm->flags & ATTO_VM_FLAG_VERBOSE) {
      printf("vm: %04lu isgt\n", vm->current_instruction_offset);
    }

    if (vm->heap[a].kind == ATTO_OBJECT_KIND_THUNK) {
      evaluate_thunk(vm, a);
    }

    if (vm->heap[b].kind == ATTO_OBJECT_KIND_THUNK) {
      evaluate_thunk(vm, b);
    }

    if ((vm->heap[a].kind != ATTO_OBJECT_KIND_NUMBER) ||
        (vm->heap[b].kind != ATTO_OBJECT_KIND_NUMBER)) {
      printf("vm: fatal: attempting to perform `isgt' on non-numeric arguments\n");
      vm->flags &= ~(ATTO_VM_FLAG_RUNNING);
      return;
    }

    vm->data_stack_size -= 1;
    vm->heap[c].kind = ATTO_OBJECT_KIND_SYMBOL;
    vm->heap[c].container.number = (vm->heap[a].container.number > vm->heap[b].container.number);
    vm->data_stack[vm->data_stack_size - 1] = c;

    vm->current_instruction_offset++;
    break;
  }

  case ATTO_VM_OP_ISGET: {
    size_t a = vm->data_stack[vm->data_stack_size-1],
           b = vm->data_stack[vm->data_stack_size-2],
           c = vm->heap_size++;

    if (vm->flags & ATTO_VM_FLAG_VERBOSE) {
      printf("vm: %04lu isget\n", vm->current_instruction_offset);
    }

    if (vm->heap[a].kind == ATTO_OBJECT_KIND_THUNK) {
      evaluate_thunk(vm, a);
    }

    if (vm->heap[b].kind == ATTO_OBJECT_KIND_THUNK) {
      evaluate_thunk(vm, b);
    }

    if ((vm->heap[a].kind != ATTO_OBJECT_KIND_NUMBER) ||
        (vm->heap[b].kind != ATTO_OBJECT_KIND_NUMBER)) {
      printf("vm: fatal: attempting to perform `isget' on non-numeric arguments\n");
      vm->flags &= ~(ATTO_VM_FLAG_RUNNING);
      return;
    }

    vm->data_stack_size -= 1;
    vm->heap[c].kind = ATTO_OBJECT_KIND_SYMBOL;
    vm->heap[c].container.number = (vm->heap[a].container.number >= vm->heap[b].container.number);
    vm->data_stack[vm->data_stack_size - 1] = c;

    vm->current_instruction_offset++;
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

  case ATTO_VM_OP_ISNULL: {
    size_t o   = vm->data_stack[vm->data_stack_size - 1],
           res = vm->heap_size++;

    if (vm->flags & ATTO_VM_FLAG_VERBOSE) {
      printf("vm: %04lu isnull\n", vm->current_instruction_offset);
    }
    
    vm->heap[res].kind = ATTO_OBJECT_KIND_SYMBOL;
    if (vm->heap[o].kind == ATTO_OBJECT_KIND_NULL) {
      vm->heap[res].container.symbol = 1;
    } else {
      vm->heap[res].container.symbol = 0;
    }

    vm->data_stack[vm->data_stack_size - 1] = res;

    vm->current_instruction_offset++;
    break;
  }

  case ATTO_VM_OP_CAR: {
    size_t list = vm->data_stack[vm->data_stack_size - 1];

    if (vm->flags & ATTO_VM_FLAG_VERBOSE) {
      printf("vm: %04lu car\n", vm->current_instruction_offset);
    }

    if (vm->heap[list].kind != ATTO_OBJECT_KIND_LIST) {
      printf("fatal: attempting to perform `car' on an invalid operand\n");
      vm->flags &= ~(ATTO_VM_FLAG_RUNNING);
      return;
    }

    vm->data_stack[vm->data_stack_size - 1] = vm->heap[list].container.list.car;

    vm->current_instruction_offset++;
    break;
  }

  case ATTO_VM_OP_CDR: {
    size_t list = vm->data_stack[vm->data_stack_size - 1];

    if (vm->flags & ATTO_VM_FLAG_VERBOSE) {
      printf("vm: %04lu cdr\n", vm->current_instruction_offset);
    }

    if (vm->heap[list].kind != ATTO_OBJECT_KIND_LIST) {
      printf("fatal: attempting to perform `cdr' on an invalid operand\n");
      vm->flags &= ~(ATTO_VM_FLAG_RUNNING);
      return;
    }

    vm->data_stack[vm->data_stack_size - 1] = vm->heap[list].container.list.cdr;

    vm->current_instruction_offset++;
    break;
  }

  case ATTO_VM_OP_CONS: {
    size_t a = vm->data_stack[vm->data_stack_size - 1],
           b = vm->data_stack[vm->data_stack_size - 2],
           c = vm->heap_size++;

    if (vm->flags & ATTO_VM_FLAG_VERBOSE) {
      printf("vm: %04lu cons\n", vm->current_instruction_offset);
    }

    vm->heap[c].kind = ATTO_OBJECT_KIND_LIST;
    vm->heap[c].container.list.car = a;
    vm->heap[c].container.list.cdr = b;

    vm->data_stack_size--;
    vm->data_stack[vm->data_stack_size - 1] = c;

    vm->current_instruction_offset++;
    break;
  }

  case ATTO_VM_OP_PUSHN: {
    if (vm->flags & ATTO_VM_FLAG_VERBOSE) {
      printf("vm: %04lu push_number %lf\n", vm->current_instruction_offset, current_instruction->container.number);
    }

    vm->heap[vm->heap_size].kind = ATTO_OBJECT_KIND_NUMBER;
    vm->heap[vm->heap_size].container.number = current_instruction->container.number;
    vm->heap_size++;

    vm->data_stack[vm->data_stack_size] = vm->heap_size - 1;
    vm->data_stack_size++;

    vm->current_instruction_offset++;
    break;
  }

  case ATTO_VM_OP_PUSHS: {
    if (vm->flags & ATTO_VM_FLAG_VERBOSE) {
      printf("vm: %04lu push_symbol %lu\n", vm->current_instruction_offset, current_instruction->container.symbol);
    }

    vm->heap[vm->heap_size].kind = ATTO_OBJECT_KIND_SYMBOL;
    vm->heap[vm->heap_size].container.symbol = current_instruction->container.symbol;
    vm->heap_size++;

    vm->data_stack[vm->data_stack_size] = vm->heap_size - 1;
    vm->data_stack_size++;

    vm->current_instruction_offset++;
    break;
  }

  case ATTO_VM_OP_PUSHL: {
    if (vm->flags & ATTO_VM_FLAG_VERBOSE) {
      printf("vm: %04lu push_lambda %lu\n", vm->current_instruction_offset, current_instruction->container.offset);
    }

    vm->heap[vm->heap_size].kind = ATTO_OBJECT_KIND_LAMBDA;
    vm->heap[vm->heap_size].container.instruction_stream_index = current_instruction->container.offset;
    vm->heap_size++;

    vm->data_stack[vm->data_stack_size] = vm->heap_size - 1;
    vm->data_stack_size++;

    vm->current_instruction_offset++;
    break;
  }

  case ATTO_VM_OP_PUSHZ: {
    size_t empty_list = vm->heap_size++;
    vm->heap[empty_list].kind = ATTO_OBJECT_KIND_NULL;

    vm->data_stack[vm->data_stack_size++] = empty_list;

    vm->current_instruction_offset += 1;
    break;
  }

  case ATTO_VM_OP_DUP:
    break;

  case ATTO_VM_OP_DROP:
    break;

  case ATTO_VM_OP_SWAP:
    break;

  case ATTO_VM_OP_GETGL: {
    if (vm->flags & ATTO_VM_FLAG_VERBOSE) {
      printf("vm: %04lu getgl %lu\n", vm->current_instruction_offset, current_instruction->container.offset);
    }

    vm->data_stack[vm->data_stack_size] = vm->data_stack[current_instruction->container.offset];
    vm->data_stack_size++;

    vm->current_instruction_offset++;
    break;
  }

  case ATTO_VM_OP_GETLC: {
    if (vm->flags & ATTO_VM_FLAG_VERBOSE) {
      printf("vm: %04lu getlc %lu\n", vm->current_instruction_offset, current_instruction->container.offset);
    }

    vm->data_stack[vm->data_stack_size] = vm->data_stack[vm->call_stack[vm->call_stack_size - 1].stack_offset_at_entrypoint + current_instruction->container.offset];
    vm->data_stack_size++;

    vm->current_instruction_offset++;
    break;
  }

  case ATTO_VM_OP_GETAG: {
    if (vm->flags & ATTO_VM_FLAG_VERBOSE) {
      printf("vm: %04lu getag %lu\n", vm->current_instruction_offset, current_instruction->container.offset);
    }

    vm->data_stack[vm->data_stack_size] = vm->data_stack[vm->call_stack[vm->call_stack_size - 1].stack_offset_at_entrypoint - current_instruction->container.offset - 1];
    vm->data_stack_size++;

    vm->current_instruction_offset++;
    break;
  }

  default:
    printf("vm: fatal: unknown opcode (0x%02x)\n", current_instruction->opcode);
    exit(1);
  }

  if (vm->flags & ATTO_VM_FLAG_VERBOSE) {
    pretty_print_stack(vm);
  }
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

    switch (vm->heap[vm->data_stack[i]].kind) {
    
    case ATTO_OBJECT_KIND_NULL:
      printf("() ");
      break;

    case ATTO_OBJECT_KIND_NUMBER:
      printf("num(%lf) ", vm->heap[vm->data_stack[i]].container.number);
      break;

    case ATTO_OBJECT_KIND_SYMBOL:
      printf("sym(%lu) ", vm->heap[vm->data_stack[i]].container.symbol);
      break;

    case ATTO_OBJECT_KIND_LIST:
      printf("list() ");
      break;

    case ATTO_OBJECT_KIND_LAMBDA:
      printf("lambda() ");
      break;

    case ATTO_OBJECT_KIND_THUNK:
      printf("thunk(%lu) ", vm->heap[vm->data_stack[i]].container.instruction_stream_index);
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

void evaluate_thunk(struct atto_vm_state *vm, size_t index)
{
  if (vm->heap[index].kind != ATTO_OBJECT_KIND_THUNK) {
    return;
  }

  atto_run_instruction_stream(vm, vm->heap[index].container.instruction_stream_index);

  /*  TODO: free linked instruction stream */

  vm->heap[index].kind = vm->heap[vm->data_stack[vm->data_stack_size - 1]].kind;
  vm->heap[index].container = vm->heap[vm->data_stack[vm->data_stack_size - 1]].container;
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

