
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

size_t compile_expression(struct atto_state *a, struct atto_environment *env,
  struct atto_instruction_stream *is, struct atto_expression *e)
{
  if (e->kind == ATTO_EXPRESSION_KIND_NUMBER_LITERAL) {
    printf("push_number %lf\n", e->container.number_literal);
    return write_push_number(is, e->container.number_literal);
  }

  if (e->kind == ATTO_EXPRESSION_KIND_SYMBOL_LITERAL) {
    printf("push_symbol %llu\n", e->container.symbol_literal);
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
}

size_t compile_reference(struct atto_state *a, struct atto_environment *env,
  struct atto_instruction_stream *is, char *name)
{
  struct atto_environment_object *eo = NULL;
  
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

  /*  if not, maybe the user defined it */
  eo = atto_find_in_environment(env, name);

  if (eo == NULL) {
    printf("syntax error: unable to find object `%s'.\n", name);
    return 0;
  }

  switch (eo->kind) {

  case ATTO_ENVIRONMENT_OBJECT_KIND_GLOBAL:
    printf("getglobal %i\n", eo->offset);
    break;

  case ATTO_ENVIRONMENT_OBJECT_KIND_LOCAL:
    printf("getlocal %i\n", eo->offset);
    break;

  case ATTO_ENVIRONMENT_OBJECT_KIND_ARGUMENT:
    printf("getargument %i\n", eo->offset);
    break;

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
  struct atto_instruction_stream *is, struct atto_application_expression *ae) {
  uint32_t i = ae->number_of_parameters - 1;

  do {
    compile_expression(a, env, is, ae->parameters[i]);
    i--;
  } while (i + 1 > 0);

  compile_reference(a, env, is, ae->identifier);
  printf("call\n");

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
  struct atto_instruction_stream *is, struct atto_lambda_expression *le) {
  uint32_t i;

  struct atto_environment *local_env = (struct atto_environment *)malloc(sizeof(struct atto_environment));
  assert(local_env != NULL);

  local_env->parent = env;

  for (i = 0; i < le->number_of_parameters; i++) {
    atto_add_to_environment(local_env, le->parameter_names[i], ATTO_ENVIRONMENT_OBJECT_KIND_ARGUMENT, i);
  }

  compile_expression(a, local_env, is, le->body);

  return 0;
}

void compile_definition(struct atto_state *a, struct atto_definition *d);

void pretty_print_instruction_stream(struct atto_instruction_stream *is)
{
  size_t i;

  printf("instruction stream size=%i alloc=%i\n", is->length, is->allocated_length);

  for (i = 0; i < is->length; i++) {
    printf("0x%02x ", is->stream[i]);
    if (i % 8 == 7) {
      printf("\n");
    }
  }

  printf("\n");
}

