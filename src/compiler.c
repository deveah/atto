
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

static struct atto_instruction_stream *allocate_instruction_stream(void)
{
  struct atto_instruction_stream *is = (struct atto_instruction_stream *)malloc(sizeof(struct atto_instruction_stream));
  assert(is != NULL);

  is->length = 0;
  is->allocated_length = 32;
  is->stream = (struct atto_instruction *)malloc(sizeof(struct atto_instruction) * is->allocated_length);
  assert(is->stream != NULL);

  return is;
}

static void check_buffer(struct atto_instruction_stream *is)
{
  if (is->allocated_length == is->length - 1) {
    is->allocated_length *= 2;
    is->stream = realloc(is->stream, sizeof(struct atto_instruction_stream) * is->allocated_length);
  }
}

static void concat_buffers(struct atto_instruction_stream *dest, struct atto_instruction_stream *src)
{
  if (dest->allocated_length - dest->length <= src->length) {
    dest->allocated_length += src->length;
    dest->stream = realloc(dest->stream, sizeof(struct atto_instruction_stream) * dest->allocated_length);
  }

  size_t i;
  for (i = 0; i < src->length; i++) {
    dest->stream[dest->length+i] = src->stream[i];
  }

  dest->length += src->length;
}

static void write_op_noarg(struct atto_instruction_stream *is, uint8_t opcode)
{
  check_buffer(is);
  is->stream[is->length].opcode = opcode;
  is->length++;
}

static void write_op_number(struct atto_instruction_stream *is, uint8_t opcode, double number)
{
  check_buffer(is);
  is->stream[is->length].opcode = opcode;
  is->stream[is->length].container.number = number;
  is->length++;
}

static void write_op_symbol(struct atto_instruction_stream *is, uint8_t opcode, uint64_t symbol)
{
  check_buffer(is);
  is->stream[is->length].opcode = opcode;
  is->stream[is->length].container.symbol = symbol;
}

static void write_op_offset(struct atto_instruction_stream *is, uint8_t opcode, size_t offset)
{
  check_buffer(is);
  is->stream[is->length].opcode = opcode;
  is->stream[is->length].container.offset = offset;
  is->length++;
}

size_t compile_expression(struct atto_state *a, struct atto_environment *env,
  struct atto_instruction_stream *is, struct atto_expression *e)
{
  (void) a;

  if (e->kind == ATTO_EXPRESSION_KIND_NUMBER_LITERAL) {
    write_op_number(is, ATTO_VM_OP_PUSHN, e->container.number_literal);
    return 1;
  }

  if (e->kind == ATTO_EXPRESSION_KIND_SYMBOL_LITERAL) {
    write_op_symbol(is, ATTO_VM_OP_PUSHS, e->container.symbol_literal);
    return 1;
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
    write_op_offset(is, ATTO_VM_OP_GETGL, eo->offset);
    break;
  }

  case ATTO_ENVIRONMENT_OBJECT_KIND_LOCAL:
    write_op_offset(is, ATTO_VM_OP_GETLC, eo->offset);
    break;

  case ATTO_ENVIRONMENT_OBJECT_KIND_ARGUMENT: {
    write_op_offset(is, ATTO_VM_OP_GETAG, eo->offset);
    break;
  }

  default:
    printf("fatal: unrecognised environment object kind: %i\n", eo->kind);
  }

  return 1;
}

size_t compile_if_expression(struct atto_state *a, struct atto_environment *env,
  struct atto_instruction_stream *is, struct atto_if_expression *ie)
{
  struct atto_instruction_stream *tis = allocate_instruction_stream(),
                                 *fis = allocate_instruction_stream();

  compile_expression(a, env, is, ie->condition_expression);
  
  compile_expression(a, env, tis, ie->true_evaluation_expression);
  compile_expression(a, env, fis, ie->false_evaluation_expression);

  printf("true branch length=%lu\n", tis->length);
  write_op_offset(is, ATTO_VM_OP_BF, is->length + tis->length + 2);
  concat_buffers(is, tis);

  printf("false branch length=%lu\n", fis->length);
  write_op_offset(is, ATTO_VM_OP_B, is->length + fis->length);
  concat_buffers(is, fis);

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
    write_op_noarg(is, ATTO_VM_OP_ADD);
    return 0;
  } else if (strcmp(name, "sub") == 0) {
    write_op_noarg(is, ATTO_VM_OP_SUB);
    return 0;
  } else if (strcmp(name, "mul") == 0) {
    write_op_noarg(is, ATTO_VM_OP_MUL);
    return 0;
  } else if (strcmp(name, "div") == 0) {
    write_op_noarg(is, ATTO_VM_OP_DIV);
    return 0;
  } else if (strcmp(name, "gt") == 0) {
    write_op_noarg(is, ATTO_VM_OP_ISGT);
    return 0;
  } else if (strcmp(name, "get") == 0) {
    write_op_noarg(is, ATTO_VM_OP_ISGET);
    return 0;
  } else if (strcmp(name, "lt") == 0) {
    write_op_noarg(is, ATTO_VM_OP_ISLT);
    return 0;
  } else if (strcmp(name, "let") == 0) {
    write_op_noarg(is, ATTO_VM_OP_ISLET);
    return 0;
  } else if (strcmp(name, "eq") == 0) {
    write_op_noarg(is, ATTO_VM_OP_ISEQ);
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
    write_op_noarg(is, ATTO_VM_OP_CAR);
    return 1;
  } else if (strcmp(name, "cdr") == 0) {
    write_op_noarg(is, ATTO_VM_OP_CDR);
    return 1;
  } else if (strcmp(name, "cons") == 0) {
    write_op_noarg(is, ATTO_VM_OP_CONS);
    return 1;
  } else if (strcmp(name, "null") == 0) {
    write_op_noarg(is, ATTO_VM_OP_ISNULL);
    return 1;
  }

  compile_reference(a, env, is, ae->identifier);
  write_op_noarg(is, ATTO_VM_OP_CALL);
  write_op_offset(is, ATTO_VM_OP_CLOSE, ae->number_of_parameters);

  return 0;
}

size_t compile_list_literal_expression(struct atto_state *a, struct atto_environment *env,
  struct atto_instruction_stream *is, struct atto_list_literal_expression *lle)
{
  int i = lle->number_of_elements;

  write_op_noarg(is, ATTO_VM_OP_PUSHZ);

  if (lle->number_of_elements == 0) {
    return 1;
  }

  do {
    /*  TODO: wrap expressions in individual thunks */
    compile_expression(a, env, is, lle->elements[i - 1]);
    write_op_noarg(is, ATTO_VM_OP_CONS);
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

  struct atto_instruction_stream *lis = allocate_instruction_stream();

  compile_expression(a, local_env, lis, le->body);
  write_op_noarg(lis, ATTO_VM_OP_RET);

  /*  add to instruction stream table */
  a->vm_state->instruction_streams[a->vm_state->number_of_instruction_streams] = lis;
  a->vm_state->number_of_instruction_streams++;

  write_op_offset(is, ATTO_VM_OP_PUSHL, a->vm_state->number_of_instruction_streams - 1);

  return 0;
}

void compile_definition(struct atto_state *a, struct atto_definition *d)
{
  struct atto_instruction_stream *is = allocate_instruction_stream();
  
  size_t definition_instruction_stream_index = a->vm_state->number_of_instruction_streams;
  a->vm_state->instruction_streams[definition_instruction_stream_index] = is;
  a->vm_state->number_of_instruction_streams++;

  atto_add_to_environment(a->global_environment, d->identifier, ATTO_ENVIRONMENT_OBJECT_KIND_GLOBAL, a->vm_state->data_stack_size);

  compile_expression(a, a->global_environment, is, d->body);
  write_op_noarg(is, ATTO_VM_OP_STOP);

  switch (d->body->kind) {
  
  /*  immediate values are compiled and then executed, in order to be saved
   *  on the stack */
  case ATTO_EXPRESSION_KIND_LAMBDA:
  case ATTO_EXPRESSION_KIND_NUMBER_LITERAL:
  case ATTO_EXPRESSION_KIND_SYMBOL_LITERAL:
  case ATTO_EXPRESSION_KIND_REFERENCE: {
    printf("running instruction stream %lu\n", definition_instruction_stream_index);
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

    a->vm_state->data_stack[a->vm_state->data_stack_size] = a->vm_state->heap_size - 1;
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
    printf("0x%02x 0x%016llx\n", is->stream[i].opcode, (unsigned long long)is->stream[i].container.number);
    if (i % 8 == 7) {
      printf("\n");
    }
  }

  printf("\n");
}

