
/*
 *  vm.c
 *  Part of Atto :: https://deveah.github.io/atto
 */

#include <assert.h>
#include <malloc.h>
#include <stdio.h>
#include <stdlib.h>

#include "atto.h"

void error(struct atto_vm_state *A, char *reason)
{
  printf("fatal error: %s\n", reason);

  destroy_state(A);
  exit(1);
}

struct atto_vm_state *allocate_state(uint32_t number_of_functions)
{
  size_t i;
  struct atto_vm_state *A;
  
  /*  allocate memory */
  A = (struct atto_vm_state *)malloc(sizeof(struct atto_vm_state));
  assert(A != NULL);

  /*  initialize a clean, empty state */
  A->number_of_functions = number_of_functions;
  A->functions           = (struct atto_vm_function **)malloc(sizeof(struct atto_vm_function *) * A->number_of_functions);

  for (i = 0; i < A->number_of_functions; i++) {
    A->functions[i] = NULL;
  }

  for (i = 0; i < ATTO_MAX_REGISTERS; i++) {
    A->registers[i] = 0;
  }

  /*  0xffffffff is a signal value, indicating that these values have not been
   *  set (indicative that the virtual machine has not been started */
  A->current_instruction_pointer.function_index    = 0xffffffff;
  A->current_instruction_pointer.instruction_index = 0xffffffff;

  return A;
}

void destroy_state(struct atto_vm_state *A)
{
  size_t i;

  for (i = 0; i < A->number_of_functions; i++) {
    destroy_function(A->functions[i]);
  }

  free(A->functions);

  free(A);
}

struct atto_vm_function *allocate_function(uint32_t number_of_arguments,
  uint32_t number_of_constants, uint32_t number_of_instructions)
{
  size_t i;
  struct atto_vm_function *f;

  /*  allocate memory */
  f = (struct atto_vm_function *)malloc(sizeof(struct atto_vm_function));
  assert(f != NULL);

  f->number_of_arguments    = number_of_arguments;
  f->number_of_constants    = number_of_constants;
  f->number_of_instructions = number_of_instructions;

  f->constants    = (uint64_t *)malloc(sizeof(uint64_t) * f->number_of_constants);
  f->instructions = (uint32_t *)malloc(sizeof(uint32_t) * f->number_of_instructions);

  for (i = 0; i < number_of_constants; i++) {
    f->constants[i] = 0;
  }

  for (i = 0; i < number_of_instructions; i++) {
    f->instructions[i] = 0;
  }
  
  return f;
}

void destroy_function(struct atto_vm_function *f)
{
  free(f->constants);
  free(f->instructions);
  free(f);
}

void print_function_details(struct atto_vm_function *f)
{
  uint32_t i;

  printf("  Arguments: %i\n", f->number_of_arguments);

  printf("  Constants: %i\n", f->number_of_constants);
  for (i = 0; i < f->number_of_constants; i++) {
    printf("    [%02x] %08lx\n", i, f->constants[i]);
  }

  printf("  Instructions: %i\n", f->number_of_instructions);
  for (i = 0; i < f->number_of_instructions; i++) {
    printf("    [%08x] %08x\n", i, f->instructions[i]);
  }

  printf("\n");
}

void print_vm_state_details(struct atto_vm_state *A)
{
  uint32_t i;

  printf("Functions: %i\n", A->number_of_functions);
  for (i = 0; i < A->number_of_functions; i++) {
    printf("Function %i:\n", i);
    print_function_details(A->functions[i]);
  }

  printf("Call stack:\n");
  if (A->call_stack_index == 0) {
    printf("  (empty)\n");
  }

  for (i = 0; i < A->call_stack_index; i++) {
    printf("  [%02x] %08x:%08x\n", i, A->call_stack[i].function_index, A->call_stack[i].instruction_index);
  }
  printf("\n");

  printf("First eight registers:\n");
  for (i = 0; i < 8; i++) {
    printf("  [%02x] %08lx\n", i, A->registers[i]);
  }

  printf("\n");
}

uint32_t perform_step(struct atto_vm_state *A)
{
  uint32_t current_function_index    = A->current_instruction_pointer.function_index;
  uint32_t current_instruction_index = A->current_instruction_pointer.instruction_index;

  /*  must be sure not to attempt to perform a step on an uninitialized
   *  state */
  if ((current_function_index == 0xffffffff) || (current_instruction_index == 0xffffffff)) {
    error(A, "Illegal state (uninitialized virtual machine)");
  }

  uint32_t current_instruction = A->functions[current_function_index]->instructions[current_instruction_index];
  printf("current instruction: [%08x] %08x; flags: %02x\n", current_instruction_index, current_instruction, A->flags);

  uint8_t opcode = current_instruction >> 24;

  switch (opcode) {

  /*  move -- 00 dest(u8) src(u8) */
  case 0x00: {
    uint8_t dest = (current_instruction & 0x00ff0000) >> 16;
    uint8_t src  = (current_instruction & 0x0000ff00) >>  8;
    A->registers[dest] = A->registers[src];
    break;
  }

  /*  load -- 01 dest(u8) index(u8) */
  case 0x01: {
    uint8_t dest  = (current_instruction & 0x00ff0000) >> 16;
    uint8_t index = (current_instruction & 0x0000ff00) >>  8;
    A->registers[dest] = A->functions[current_function_index]->constants[index];
    break;
  }

  /*  add -- 02 dest(u8) op1(u8) op2(u8) */
  case 0x02: {
    uint8_t dest = (current_instruction & 0x00ff0000) >> 16;
    uint8_t op1  = (current_instruction & 0x0000ff00) >>  8;
    uint8_t op2  = (current_instruction & 0x000000ff);
    A->registers[dest] = A->registers[op1] + A->registers[op2];
    break;
  }
  
  /*  sub -- 03 dest(u8) op1(u8) op2(u8) */
  case 0x03: {
    uint8_t dest = (current_instruction & 0x00ff0000) >> 16;
    uint8_t op1  = (current_instruction & 0x0000ff00) >>  8;
    uint8_t op2  = (current_instruction & 0x000000ff);
    A->registers[dest] = A->registers[op1] - A->registers[op2];
    break;
  }

  /*  mul -- 04 dest(u8) op1(u8) op2(u8) */
  case 0x04: {
    uint8_t dest = (current_instruction & 0x00ff0000) >> 16;
    uint8_t op1  = (current_instruction & 0x0000ff00) >>  8;
    uint8_t op2  = (current_instruction & 0x000000ff);
    A->registers[dest] = A->registers[op1] * A->registers[op2];
    break;
  }

  /*  div -- 05 dest(u8) op1(u8) op2(u8) */
  case 0x05: {
    uint8_t dest = (current_instruction & 0x00ff0000) >> 16;
    uint8_t op1  = (current_instruction & 0x0000ff00) >>  8;
    uint8_t op2  = (current_instruction & 0x000000ff);
    A->registers[dest] = A->registers[op1] / A->registers[op2];
    break;
  }

  /*  jmp -- 10 index(u8) */
  case 0x10: {
    uint8_t index = (current_instruction & 0x00ff0000) >> 16;
    A->current_instruction_pointer.instruction_index = A->functions[current_function_index]->constants[index];
    break;
  }

  /*  conditional-jmp -- 11 index(u8) mask(u8) */
  case 0x11: {
    uint8_t index = (current_instruction & 0x00ff0000) >> 16;
    uint8_t mask  = (current_instruction & 0x0000ff00) >>  8;
    if (A->flags == mask) {
      /*  the -1 compensates for the automatic instruction pointer advancement */
      A->current_instruction_pointer.instruction_index = A->functions[current_function_index]->constants[index] - 1;
    }
    break;
  }

  /*  test-reg -- 20 op1(u8) op2(u8); sets flags */
  case 0x20: {
    uint8_t  op1  = (current_instruction & 0x00ff0000) >> 16;
    uint8_t  op2  = (current_instruction & 0x0000ff00) >>  8;
    uint64_t reg1 = A->registers[op1];
    uint64_t reg2 = A->registers[op2];

    A->flags = 0x00;

    if (reg1 == reg2) {
      A->flags |= ATTO_VMFLAG_EQUAL;
    }
    if (reg1 >  reg2) {
      A->flags |= ATTO_VMFLAG_GREATER;
    }
    if (reg1 <  reg2) {
      A->flags |= ATTO_VMFLAG_LESSER;
    }

    break;
  }

  /*  call -- f0 index(u8) */
  case 0xf0: {
    uint8_t index = (current_instruction & 0x00ff0000) >> 16;
    
    if (A->call_stack_index + 1 >= ATTO_CALLSTACK_DEPTH) {
      error(A, "Call stack overflow!");
    }

    /*  save the current instruction pointer on the call stack */
    A->call_stack[A->call_stack_index].function_index    = current_function_index;
    A->call_stack[A->call_stack_index].instruction_index = current_instruction_index;
    A->call_stack_index++;

    A->current_instruction_pointer.function_index    = A->functions[current_function_index]->constants[index];
    A->current_instruction_pointer.instruction_index = 0;

    break;
  }

  /*  return -- f8 */
  case 0xf8: {
    if (A->call_stack_index == 0) {
      print_vm_state_details(A);
      error(A, "Reached bottom of call stack. Terminating VM.");
      return 0;
    }

    A->call_stack_index--;
    A->current_instruction_pointer.function_index    = A->call_stack[A->call_stack_index].function_index;
    A->current_instruction_pointer.instruction_index = A->call_stack[A->call_stack_index].instruction_index;

    current_function_index    = A->current_instruction_pointer.function_index;
    current_instruction_index = A->current_instruction_pointer.instruction_index;
    break;
  }

  default:
    error(A, "Illegal instruction");
  }

  A->current_instruction_pointer.instruction_index++;
  
  if (A->current_instruction_pointer.instruction_index >= A->functions[current_function_index]->number_of_instructions) {
    error(A, "Runaway function");
  }
}

