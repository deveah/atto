
/*
 *  attoi.h
 *  the Atto Bytecode Interpteter :: https://deveah.github.io/atto
 */

#include <stdint.h>

struct atto_vm_function {
  uint8_t number_of_arguments;

  uint32_t number_of_constants;
  uint64_t *constants;

  uint32_t number_of_instructions;
  uint32_t *instructions;
};

struct atto_vm_instruction_pointer {
  uint32_t function_index;
  uint32_t instruction_index;
};

struct atto_vm_state {
  uint32_t number_of_functions;
  struct atto_vm_function **functions;

  #define ATTO_MAX_REGISTERS 256
  uint64_t registers[ATTO_MAX_REGISTERS];

  #define ATTO_CALLSTACK_DEPTH 256
  struct atto_vm_instruction_pointer call_stack[ATTO_CALLSTACK_DEPTH];
  uint32_t call_stack_index;

  struct atto_vm_instruction_pointer current_instruction_pointer;

  #define ATTO_VMFLAG_EQUAL   (1<<0)
  #define ATTO_VMFLAG_GREATER (1<<1)
  #define ATTO_VMFLAG_LESSER  (1<<2)
  uint8_t flags;
};

void error(struct atto_vm_state *A, char *reason);
struct atto_vm_state *allocate_state(uint32_t number_of_functions);
void destroy_state(struct atto_vm_state *A);
struct atto_vm_function *allocate_function(uint32_t number_of_arguments,
  uint32_t number_of_constants, uint32_t number_of_instructions);
void destroy_function(struct atto_vm_function *f);
uint32_t perform_step(struct atto_vm_state *A);

