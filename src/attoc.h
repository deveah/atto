
/*
 *  attoc.h
 *  the Atto Compiler :: https://deveah.github.io/atto
 */

#include <stdint.h>

#pragma once

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

