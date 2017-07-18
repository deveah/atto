
/*
 *  atto.h
 *  Part of Atto :: https://deveah.github.io/atto
 */

#include <stdint.h>

#pragma once

/*
 *  parser.c
 */

#define ATTO_MAX_TOKEN_LENGTH 256

struct atto_token {
  char *token;

  #define ATTO_TOKEN_OPEN_PAREN   0
  #define ATTO_TOKEN_CLOSED_PAREN 1
  #define ATTO_TOKEN_IDENTIFIER   2
  #define ATTO_TOKEN_NUMBER       3
  uint32_t kind;

  struct atto_token *next;
};

struct atto_ast_node {
  #define ATTO_AST_NODE_IDENTIFIER  0
  #define ATTO_AST_NODE_NUMBER      1
  #define ATTO_AST_NODE_LIST        2
  uint32_t kind;

  union {
    char *identifier;
    double number;
    struct atto_ast_node *list;
  } container;

  struct atto_ast_node *next;
};

struct atto_token *atto_lex_string(const char *string);
struct atto_ast_node *atto_parse_token_list(struct atto_token *root, struct atto_token **left);
void destroy_token_list(struct atto_token *head);
void destroy_ast(struct atto_ast_node *root);

/*
 *  vm.c
 */

struct atto_vm_function {
  uint8_t number_of_arguments;

  uint32_t number_of_constants;
  uint64_t *constants;

  uint32_t number_of_instructions;
  uint32_t *instructions;
};

struct atto_vm_state {
  uint32_t number_of_functions;
  struct atto_vm_function **functions;

  #define ATTO_MAX_REGISTERS 256
  uint64_t registers[ATTO_MAX_REGISTERS];

  uint32_t current_function;
  uint32_t current_instruction;

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

