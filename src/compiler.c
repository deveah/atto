
/*
 *  compiler.c
 *  part of Atto :: https://github.com/deveah/atto
 */

#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "vm.h"
#include "state.h"
#include "compiler.h"

static void check_buffer(struct atto_instruction_stream *is, size_t length)
{
  if (is->allocated_length - is->length < length) {
    is->allocated_length *= 2;
    is->stream = realloc(is->stream, sizeof(uint8_t) * is->allocated_length);
  }
}

static size_t write_push_number(struct atto_instruction_stream *is, double number)
{
  const uint8_t opcode = ATTO_VM_OP_PUSHN;
  const size_t instruction_size = sizeof(uint8_t) + sizeof(double);

  check_buffer(is, instruction_size);

  memcpy(is->stream + is->length, &opcode, sizeof(uint8_t));
  is->length += sizeof(uint8_t);

  memcpy(is->stream + is->length, &number, sizeof(double));
  is->length += sizeof(double);

  return instruction_size;
}

static size_t write_add(struct atto_instruction_stream *is)
{
  const uint8_t opcode = ATTO_VM_OP_ADD;
  const size_t instruction_size = sizeof(uint8_t);

  check_buffer(is, instruction_size);

  memcpy(is->stream + is->length, &opcode, sizeof(uint8_t));
  is->length += sizeof(uint8_t);

  return instruction_size;
}

static size_t write_get_global(struct atto_instruction_stream *is, size_t offset)
{
  const uint8_t opcode = ATTO_VM_OP_GETGL;
  const size_t instruction_size = sizeof(uint8_t) + sizeof(size_t);

  check_buffer(is, instruction_size);

  memcpy(is->stream + is->length, &opcode, sizeof(uint8_t));
  is->length += sizeof(uint8_t);

  memcpy(is->stream + is->length, &offset, sizeof(size_t));
  is->length += sizeof(size_t);

  return instruction_size;
}

static size_t write_get_argument(struct atto_instruction_stream *is, size_t offset)
{
  const uint8_t opcode = ATTO_VM_OP_GETAG;
  const size_t instruction_size = sizeof(uint8_t) + sizeof(size_t);

  check_buffer(is, instruction_size);

  memcpy(is->stream + is->length, &opcode, sizeof(uint8_t));
  is->length += sizeof(uint8_t);

  memcpy(is->stream + is->length, &offset, sizeof(size_t));
  is->length += sizeof(size_t);

  return instruction_size;
}

static size_t write_call(struct atto_instruction_stream *is)
{
  const uint8_t opcode = ATTO_VM_OP_CALL;
  const size_t instruction_size = sizeof(uint8_t);

  check_buffer(is, instruction_size);

  memcpy(is->stream + is->length, &opcode, sizeof(uint8_t));
  is->length += sizeof(uint8_t);

  return instruction_size;
}

static size_t write_return(struct atto_instruction_stream *is)
{
  const uint8_t opcode = ATTO_VM_OP_RET;
  const size_t instruction_size = sizeof(uint8_t);

  check_buffer(is, instruction_size);

  memcpy(is->stream + is->length, &opcode, sizeof(uint8_t));
  is->length += sizeof(uint8_t);

  return instruction_size;
}

static size_t write_close(struct atto_instruction_stream *is, size_t count)
{
  const uint8_t opcode = ATTO_VM_OP_CLOSE;
  const size_t instruction_size = sizeof(uint8_t) + sizeof(size_t);

  check_buffer(is, instruction_size);

  memcpy(is->stream + is->length, &opcode, sizeof(uint8_t));
  is->length += sizeof(uint8_t);

  memcpy(is->stream + is->length, &count, sizeof(size_t));
  is->length += sizeof(size_t);

  return instruction_size;
}

size_t compile_expression(struct atto_state *a, struct atto_environment *env,
  struct atto_instruction_stream *is, struct atto_expression *e)
{
  (void) a;

  if (e->kind == ATTO_EXPRESSION_KIND_NUMBER_LITERAL) {
    return write_push_number(is, e->container.number_literal);
  }

  if (e->kind == ATTO_EXPRESSION_KIND_SYMBOL_LITERAL) {
    printf("push_symbol %lu\n", e->container.symbol_literal);
    return (1 + sizeof(uint64_t));
  }

  if (e->kind == ATTO_EXPRESSION_KIND_REFERENCE) {
    return compile_reference(a, env, is, e->container.reference_identifier);
  }

  if (e->kind == ATTO_EXPRESSION_KIND_LIST_LITERAL) {
    return compile_list_literal_expression(a, env, is, e->container.list_literal_expression);
  }

  if (e->kind == ATTO_EXPRESSION_KIND_LAMBDA) {
    return compile_lambda_expression(a, env, is, e->container.lambda_expression);
  }

  if (e->kind == ATTO_EXPRESSION_KIND_IF) {
    return compile_if_expression(a, env, is, e->container.if_expression);
  }

  if (e->kind == ATTO_EXPRESSION_KIND_APPLICATION) {
    return compile_application_expression(a, env, is, e->container.application_expression);
  }

  printf("fatal: unknown expression type `%i'.\n", e->kind);
  return 0;
}

size_t compile_reference(struct atto_state *a, struct atto_environment *env,
  struct atto_instruction_stream *is, char *name)
{
  (void) a;

  struct atto_environment_object *eo = NULL;
  
  eo = atto_find_in_environment(env, name);

  if (eo == NULL) {
    printf("syntax error: unable to find object `%s'.\n", name);
    return 0;
  }

  switch (eo->kind) {

  case ATTO_ENVIRONMENT_OBJECT_KIND_GLOBAL:
    return write_get_global(is, eo->offset);

  case ATTO_ENVIRONMENT_OBJECT_KIND_LOCAL:
    printf("getlocal %lu\n", eo->offset);
    break;

  case ATTO_ENVIRONMENT_OBJECT_KIND_ARGUMENT:
    return write_get_argument(is, eo->offset);

  default:
    printf("fatal: unrecognised environment object kind: %i\n", eo->kind);
  }

  return (1 + sizeof(uint64_t));
}

size_t compile_if_expression(struct atto_state *a, struct atto_environment *env,
  struct atto_instruction_stream *is, struct atto_if_expression *ie)
{
  compile_expression(a, env, is, ie->condition_expression);
  printf("brf temp\n");
  compile_expression(a, env, is, ie->true_evaluation_expression);
  printf("ret\n");
  printf("temp:\n");
  compile_expression(a, env, is, ie->false_evaluation_expression);
  printf("ret\n");

  return 0;
}

size_t compile_application_expression(struct atto_state *a, struct atto_environment *env,
  struct atto_instruction_stream *is, struct atto_application_expression *ae)
{
  char *name = ae->identifier;
  uint32_t i = ae->number_of_parameters - 1;

  do {
    compile_expression(a, env, is, ae->parameters[i]);
    i--;
  } while (i + 1 > 0);

  /*  we first see if it's a built-in function */
  if (strcmp(name, "add") == 0) {
    return write_add(is);
  } else if (strcmp(name, "sub") == 0) {
    printf("sub\n");
    return 1;
  } else if (strcmp(name, "mul") == 0) {
    printf("mul\n");
    return 1;
  } else if (strcmp(name, "div") == 0) {
    printf("div\n");
    return 1;
  } else if (strcmp(name, "gt") == 0) {
    printf("isgt\n");
    return 1;
  } else if (strcmp(name, "get") == 0) {
    printf("isget\n");
    return 1;
  } else if (strcmp(name, "lt") == 0) {
    printf("islt\n");
    return 1;
  } else if (strcmp(name, "let") == 0) {
    printf("islet\n");
    return 1;
  } else if (strcmp(name, "eq") == 0) {
    printf("iseq\n");
    return 1;
  } else if (strcmp(name, "is") == 0) {
    printf("isseq\n");
    return 1;
  } else if (strcmp(name, "and") == 0) {
    printf("and\n");
    return 1;
  } else if (strcmp(name, "or") == 0) {
    printf("or\n");
    return 1;
  } else if (strcmp(name, "not") == 0) {
    printf("not\n");
    return 1;
  }

  compile_reference(a, env, is, ae->identifier);
  write_call(is);

  return 0;
}

size_t compile_list_literal_expression(struct atto_state *a, struct atto_environment *env,
  struct atto_instruction_stream *is, struct atto_list_literal_expression *lle)
{
  uint32_t i = lle->number_of_elements - 1;

  printf("pushz\n");

  do {
    compile_expression(a, env, is, lle->elements[i]);
    printf("cons\n");
    i--;
  } while (i + 1 > 0);

  return 0;
}

size_t compile_lambda_expression(struct atto_state *a, struct atto_environment *env,
  struct atto_instruction_stream *is, struct atto_lambda_expression *le)
{
  (void) is;

  uint32_t i;

  struct atto_environment *local_env = (struct atto_environment *)malloc(sizeof(struct atto_environment));
  assert(local_env != NULL);

  local_env->parent = env;

  for (i = 0; i < le->number_of_parameters; i++) {
    atto_add_to_environment(local_env, le->parameter_names[i], ATTO_ENVIRONMENT_OBJECT_KIND_ARGUMENT, i);
  }

  struct atto_instruction_stream *lis = (struct atto_instruction_stream *)malloc(sizeof(struct atto_instruction_stream));
  lis->length = 0;
  lis->allocated_length = 32;
  lis->stream = (uint8_t *)malloc(sizeof(uint8_t) * lis->allocated_length);

  size_t expression_length = compile_expression(a, local_env, lis, le->body);
  size_t close_length = write_close(lis, le->number_of_parameters);
  size_t return_length = write_return(lis);

  /*  add to instruction stream table */
  a->vm_state->instruction_streams[a->vm_state->number_of_instruction_streams] = lis;
  a->vm_state->number_of_instruction_streams++;

  return expression_length + close_length + return_length;
}

void compile_definition(struct atto_state *a, struct atto_definition *d)
{
  struct atto_instruction_stream *is = NULL;
  
  is = (struct atto_instruction_stream *)malloc(sizeof(struct atto_instruction_stream));
  is->length = 0;
  is->allocated_length = 32;
  is->stream = (uint8_t *)malloc(sizeof(uint8_t) * is->allocated_length);

  a->vm_state->instruction_streams[a->vm_state->number_of_instruction_streams] = is;
  a->vm_state->number_of_instruction_streams++;

  compile_expression(a, a->global_environment, is, d->body);

  switch (d->body->kind) {
  
  /*  immediate values are compiled and then executed, in order to be saved
   *  on the stack */
  case ATTO_EXPRESSION_KIND_NUMBER_LITERAL:
  case ATTO_EXPRESSION_KIND_SYMBOL_LITERAL:
  case ATTO_EXPRESSION_KIND_REFERENCE: {
    a->vm_state->current_instruction_stream_index = a->vm_state->number_of_instruction_streams - 1;
    a->vm_state->current_instruction_offset = 0;
    atto_run_vm(a->vm_state);
    break;
  }

  /*  list literals, if expressions and function applications are the subjects
   *  of lazy evaluation */
  case ATTO_EXPRESSION_KIND_LIST_LITERAL:
  case ATTO_EXPRESSION_KIND_IF:
  case ATTO_EXPRESSION_KIND_APPLICATION: {
    a->vm_state->heap[a->vm_state->heap_size].kind = ATTO_OBJECT_KIND_THUNK;
    a->vm_state->heap[a->vm_state->heap_size].container.instruction_stream_index = a->vm_state->number_of_instruction_streams - 1;
    a->vm_state->heap_size++;

    a->vm_state->data_stack[a->vm_state->data_stack_size] = &a->vm_state->heap[a->vm_state->heap_size - 1];
    a->vm_state->data_stack_size++;
    break;
  }

  case ATTO_EXPRESSION_KIND_LAMBDA: {
    a->vm_state->heap[a->vm_state->heap_size].kind = ATTO_OBJECT_KIND_LAMBDA;
    a->vm_state->heap[a->vm_state->heap_size].container.instruction_stream_index = a->vm_state->number_of_instruction_streams - 1;
    a->vm_state->heap_size++;

    a->vm_state->data_stack[a->vm_state->data_stack_size] = &a->vm_state->heap[a->vm_state->heap_size - 1];
    a->vm_state->data_stack_size++;
    break;
  }

  default:
    break;
  }

  atto_add_to_environment(a->global_environment, d->identifier, ATTO_ENVIRONMENT_OBJECT_KIND_GLOBAL, a->vm_state->data_stack_size - 1);
}

void pretty_print_instruction_stream(struct atto_instruction_stream *is)
{
  size_t i;

  printf("instruction stream size=%lu alloc=%lu\n", is->length, is->allocated_length);

  for (i = 0; i < is->length; i++) {
    printf("0x%02x ", is->stream[i]);
    if (i % 8 == 7) {
      printf("\n");
    }
  }

  printf("\n");
}

