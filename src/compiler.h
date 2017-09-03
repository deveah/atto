
/*
 *  compiler.h
 *  part of Atto :: https://github.com/deveah/atto
 */

#include "state.h"
#include "parser.h"
#include "vm.h"

#pragma once

size_t compile_expression(struct atto_state *a, struct atto_environment *env,
  struct atto_instruction_stream *is, struct atto_expression *e);

size_t compile_reference(struct atto_state *a, struct atto_environment *env,
  struct atto_instruction_stream *is, char *name);

size_t compile_if_expression(struct atto_state *a, struct atto_environment *env,
  struct atto_instruction_stream *is, struct atto_if_expression *ie);

size_t compile_application_expression(struct atto_state *a, struct atto_environment *env,
  struct atto_instruction_stream *is, struct atto_application_expression *ae);

size_t compile_list_literal_expression(struct atto_state *a, struct atto_environment *env,
  struct atto_instruction_stream *is, struct atto_list_literal_expression *lle);

size_t compile_lambda_expression(struct atto_state *a, struct atto_environment *env,
  struct atto_instruction_stream *is, struct atto_lambda_expression *le);

void compile_definition(struct atto_state *a, struct atto_definition *d);

void pretty_print_instruction_stream(struct atto_instruction_stream *is);

