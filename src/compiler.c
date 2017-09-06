
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
    is->allocated_length += length;
    is->stream = realloc(is->stream, sizeof(uint8_t) * is->allocated_length);
  }
}

static size_t write_opcode(struct atto_instruction_stream *is, uint8_t opcode)
{
  check_buffer(is, sizeof(uint8_t));
  
  memcpy(is->stream + is->length, &opcode, sizeof(uint8_t));
  is->length += sizeof(uint8_t);

  return sizeof(uint8_t);
}

static size_t write_number(struct atto_instruction_stream *is, double number)
{
  check_buffer(is, sizeof(double));

  memcpy(is->stream + is->length, &number, sizeof(double));
  is->length += sizeof(double);

  return sizeof(double);
}

static size_t write_symbol(struct atto_instruction_stream *is, uint64_t symbol)
{
  check_buffer(is, sizeof(uint64_t));

  memcpy(is->stream + is->length, &symbol, sizeof(uint64_t));
  is->length += sizeof(uint64_t);

  return sizeof(uint64_t);
}

static size_t write_offset(struct atto_instruction_stream *is, size_t offset)
{
  check_buffer(is, sizeof(size_t));

  memcpy(is->stream + is->length, &offset, sizeof(size_t));
  is->length += sizeof(size_t);

  return sizeof(size_t);
}

size_t compile_expression(struct atto_state *a, struct atto_environment *env,
  struct atto_instruction_stream *is, struct atto_expression *e)
{
  (void) a;

  if (e->kind == ATTO_EXPRESSION_KIND_NUMBER_LITERAL) {
    size_t os = write_opcode(is, ATTO_VM_OP_PUSHN);
    size_t ns = write_number(is, e->container.number_literal);
    return (os + ns);
  }

  if (e->kind == ATTO_EXPRESSION_KIND_SYMBOL_LITERAL) {
    size_t os = write_opcode(is, ATTO_VM_OP_PUSHS);
    size_t ss = write_symbol(is, e->container.symbol_literal);
    return (os + ss);
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

  case ATTO_ENVIRONMENT_OBJECT_KIND_GLOBAL: {
    write_opcode(is, ATTO_VM_OP_GETGL);
    write_offset(is, eo->offset);
    break;
  }

  case ATTO_ENVIRONMENT_OBJECT_KIND_LOCAL:
    printf("getlocal %lu\n", eo->offset);
    break;

  case ATTO_ENVIRONMENT_OBJECT_KIND_ARGUMENT: {
    write_opcode(is, ATTO_VM_OP_GETAG);
    write_offset(is, eo->offset);
    break;
  }

  default:
    printf("fatal: unrecognised environment object kind: %i\n", eo->kind);
  }

  return (1 + sizeof(uint64_t));
}

size_t compile_if_expression(struct atto_state *a, struct atto_environment *env,
  struct atto_instruction_stream *is, struct atto_if_expression *ie)
{
  struct atto_instruction_stream *tis = (struct atto_instruction_stream *)malloc(sizeof(struct atto_instruction_stream));
  tis->length = 0;
  tis->allocated_length = 32;
  tis->stream = (uint8_t *)malloc(sizeof(uint8_t) * tis->allocated_length);

  struct atto_instruction_stream *fis = (struct atto_instruction_stream *)malloc(sizeof(struct atto_instruction_stream));
  fis->length = 0;
  fis->allocated_length = 32;
  fis->stream = (uint8_t *)malloc(sizeof(uint8_t) * fis->allocated_length);

  compile_expression(a, env, is, ie->condition_expression);
  
  compile_expression(a, env, tis, ie->true_evaluation_expression);
  compile_expression(a, env, fis, ie->false_evaluation_expression);

  write_opcode(is, ATTO_VM_OP_BF);

  /*  TODO explain black magic */
  write_offset(is, is->length + tis->length + sizeof(size_t) + sizeof(uint8_t) + sizeof(size_t));

  check_buffer(is, tis->length);
  memcpy(is->stream + is->length, tis->stream, tis->length);
  is->length += tis->length;

  check_buffer(is, sizeof(uint8_t) + sizeof(size_t));
  write_opcode(is, ATTO_VM_OP_B);
  write_offset(is, is->length + fis->length + sizeof(size_t));

  check_buffer(is, fis->length);
  memcpy(is->stream + is->length, fis->stream, fis->length);
  is->length += fis->length;

  return 0;
}

size_t compile_application_expression(struct atto_state *a, struct atto_environment *env,
  struct atto_instruction_stream *is, struct atto_application_expression *ae)
{
  char *name = ae->identifier;
  int i = ae->number_of_parameters;

  do {
    compile_expression(a, env, is, ae->parameters[i-1]);
    i--;
  } while (i > 0);

  /*  we first see if it's a built-in function */
  if (strcmp(name, "add") == 0) {
    write_opcode(is, ATTO_VM_OP_ADD);
    return 0;
  } else if (strcmp(name, "sub") == 0) {
    write_opcode(is, ATTO_VM_OP_SUB);
    return 0;
  } else if (strcmp(name, "mul") == 0) {
    write_opcode(is, ATTO_VM_OP_MUL);
    return 0;
  } else if (strcmp(name, "div") == 0) {
    write_opcode(is, ATTO_VM_OP_DIV);
    return 0;
  } else if (strcmp(name, "gt") == 0) {
    write_opcode(is, ATTO_VM_OP_ISGT);
    return 0;
  } else if (strcmp(name, "get") == 0) {
    write_opcode(is, ATTO_VM_OP_ISGET);
    return 0;
  } else if (strcmp(name, "lt") == 0) {
    write_opcode(is, ATTO_VM_OP_ISLT);
    return 0;
  } else if (strcmp(name, "let") == 0) {
    write_opcode(is, ATTO_VM_OP_ISLET);
    return 0;
  } else if (strcmp(name, "eq") == 0) {
    write_opcode(is, ATTO_VM_OP_ISEQ);
    return 0;
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
  } else if (strcmp(name, "car") == 0) {
    write_opcode(is, ATTO_VM_OP_CAR);
    return 1;
  } else if (strcmp(name, "cdr") == 0) {
    write_opcode(is, ATTO_VM_OP_CDR);
    return 1;
  } else if (strcmp(name, "cons") == 0) {
    write_opcode(is, ATTO_VM_OP_CONS);
    return 1;
  } else if (strcmp(name, "null") == 0) {
    write_opcode(is, ATTO_VM_OP_ISNULL);
    return 1;
  }

  compile_reference(a, env, is, ae->identifier);
  write_opcode(is, ATTO_VM_OP_CALL);
  write_opcode(is, ATTO_VM_OP_CLOSE);
  write_offset(is, ae->number_of_parameters);

  return 0;
}

size_t compile_list_literal_expression(struct atto_state *a, struct atto_environment *env,
  struct atto_instruction_stream *is, struct atto_list_literal_expression *lle)
{
  int i = lle->number_of_elements;

  write_opcode(is, ATTO_VM_OP_PUSHZ);

  if (lle->number_of_elements == 0) {
    return 1;
  }

  do {
    /*  TODO: wrap expressions in individual thunks */
    compile_expression(a, env, is, lle->elements[i - 1]);
    write_opcode(is, ATTO_VM_OP_CONS);
    i--;
  } while (i > 0);

  return 0;
}

size_t compile_lambda_expression(struct atto_state *a, struct atto_environment *env,
  struct atto_instruction_stream *is, struct atto_lambda_expression *le)
{
  uint32_t i = le->number_of_parameters;

  struct atto_environment *local_env = (struct atto_environment *)malloc(sizeof(struct atto_environment));
  assert(local_env != NULL);

  local_env->parent = env;
  local_env->head = NULL;

  do {
    atto_add_to_environment(local_env, le->parameter_names[le->number_of_parameters - i], ATTO_ENVIRONMENT_OBJECT_KIND_ARGUMENT, le->number_of_parameters - i);
    i--;
  } while (i > 0);

  struct atto_instruction_stream *lis = (struct atto_instruction_stream *)malloc(sizeof(struct atto_instruction_stream));
  lis->length = 0;
  lis->allocated_length = 32;
  lis->stream = (uint8_t *)malloc(sizeof(uint8_t) * lis->allocated_length);

  size_t expression_length = compile_expression(a, local_env, lis, le->body);
  size_t return_length = write_opcode(lis, ATTO_VM_OP_RET);

  /*  add to instruction stream table */
  a->vm_state->instruction_streams[a->vm_state->number_of_instruction_streams] = lis;
  a->vm_state->number_of_instruction_streams++;

  write_opcode(is, ATTO_VM_OP_PUSHL);
  write_offset(is, a->vm_state->number_of_instruction_streams - 1);

  return expression_length + return_length;
}

void compile_definition(struct atto_state *a, struct atto_definition *d)
{
  struct atto_instruction_stream *is = NULL;
  
  is = (struct atto_instruction_stream *)malloc(sizeof(struct atto_instruction_stream));
  is->length = 0;
  is->allocated_length = 32;
  is->stream = (uint8_t *)malloc(sizeof(uint8_t) * is->allocated_length);

  size_t definition_instruction_stream_index = a->vm_state->number_of_instruction_streams;
  a->vm_state->instruction_streams[definition_instruction_stream_index] = is;
  a->vm_state->number_of_instruction_streams++;

  atto_add_to_environment(a->global_environment, d->identifier, ATTO_ENVIRONMENT_OBJECT_KIND_GLOBAL, a->vm_state->data_stack_size);

  compile_expression(a, a->global_environment, is, d->body);
  write_opcode(is, ATTO_VM_OP_STOP);

  switch (d->body->kind) {
  
  /*  immediate values are compiled and then executed, in order to be saved
   *  on the stack */
  case ATTO_EXPRESSION_KIND_LAMBDA:
  case ATTO_EXPRESSION_KIND_NUMBER_LITERAL:
  case ATTO_EXPRESSION_KIND_SYMBOL_LITERAL:
  case ATTO_EXPRESSION_KIND_REFERENCE: {
    printf("running instruction stream %i\n", definition_instruction_stream_index);
    atto_run_instruction_stream(a->vm_state, definition_instruction_stream_index);
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

  default:
    break;
  }

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

