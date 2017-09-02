
/*
 *  lexer.h
 *  part of Atto :: https://github.com/deveah/atto
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
  #define ATTO_TOKEN_SYMBOL       4
  uint32_t kind;

  struct atto_token *next;
};


struct atto_token *atto_lex_string(const char *string);
void destroy_token_list(struct atto_token *head);

