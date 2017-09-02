
/*
 *  lexer.c
 *  part of Atto :: https://github.com/deveah/atto
 */

#include <stdlib.h>
#include <assert.h>
#include <stdint.h>
#include <string.h>

#include "lexer.h"

/*
 *  returns whether a character is a digit
 */
static int is_digit(char c)
{
  return ((c >= '0') && (c <= '9'));
}

/*
 *  returns whether a character is a letter
 */
static int is_letter(char c)
{
  return (((c >= 'a') && (c <= 'z')) || ((c >= 'A') && (c <= 'Z')));
}

/*
 *  returns whether a character is alphanumeric
 */
static int is_alnum(char c)
{
  return (is_letter(c) || is_digit(c));
}

/*
 *  returns whether a character is whitespace
 */
static int is_whitespace(char c)
{
  return ((c == ' ') || (c == '\n') || (c == '\t'));
}

/*
 *  allocates an atto_token struct, also allocating space for the token's
 *  string storage
 */
static struct atto_token *allocate_token(char *token, uint32_t kind)
{
  struct atto_token *t = (struct atto_token *)malloc(sizeof(struct atto_token));

  assert(t != NULL);

  if (token == NULL) {
    t->token = NULL;
  } else {
    t->token = (char *)malloc(sizeof(char) * (strlen(token) + 1));
    assert(t->token != NULL);
    strcpy(t->token, token);
  }

  t->kind = kind;
  t->next = NULL;

  return t;
}

/*
 *  performs lexical analysis on a given string, and returns a list of tokens
 */
struct atto_token *atto_lex_string(const char *string)
{
  struct atto_token *token_list = NULL;
  struct atto_token *last_token = NULL;

  char temp_token[ATTO_MAX_TOKEN_LENGTH];
  size_t temp_token_index = 0;

  while (*string) {

    /*  whitespace is ignored */
    while ((*string != '\0') && is_whitespace(*string)) {
      string++;
    }
    
    /*  number literals start with either a digit, or a minus sign */
    if (is_digit(*string) || (*string == '-')) {
      struct atto_token *number_literal = NULL;

      while (!is_whitespace(*string) && (*string != ')')) {
        temp_token[temp_token_index++] = *string++;
      }
      temp_token[temp_token_index] = 0;
      temp_token_index = 0;

      number_literal = allocate_token(temp_token, ATTO_TOKEN_NUMBER);
      if (token_list == NULL) {
        token_list = number_literal;
        last_token = number_literal;
      } else {
        last_token->next = number_literal;
        last_token       = number_literal;
      }
    }

    /*  symbol literals start with a colon sign, and may contain any
     *  combination of letters, digits, and dashes */
    if (*string == ':') {
      struct atto_token *symbol_literal = NULL;

      string++;
      while (is_alnum(*string) || (*string == '-')) {
        temp_token[temp_token_index++] = *string++;
      }
      temp_token[temp_token_index] = 0;
      temp_token_index = 0;
      
      symbol_literal = allocate_token(temp_token, ATTO_TOKEN_SYMBOL);
      if (token_list == NULL) {
        token_list = symbol_literal;
        last_token = symbol_literal;
      } else {
        last_token->next = symbol_literal;
        last_token       = symbol_literal;
      }
    }

    /*  identifiers start with a letter, and may contain any combination of
     *  letters, digits, and dashes */
    if (is_letter(*string)) {
      struct atto_token *identifier = NULL;

      while (is_alnum(*string) || (*string == '-')) {
        temp_token[temp_token_index++] = *string++;
      }
      temp_token[temp_token_index] = 0;
      temp_token_index = 0;
      
      identifier = allocate_token(temp_token, ATTO_TOKEN_IDENTIFIER);
      if (token_list == NULL) {
        token_list = identifier;
        last_token = identifier;
      } else {
        last_token->next = identifier;
        last_token       = identifier;
      }
    }

    /*  an open paranthesis marks the beginning of a list */
    if (*string == '(') {
      struct atto_token *open_paren = allocate_token(NULL, ATTO_TOKEN_OPEN_PAREN);
      if (token_list == NULL) {
        token_list = open_paren;
        last_token = open_paren;
      } else {
        last_token->next = open_paren;
        last_token       = open_paren;
      }

      string++;
    }

    /*  a closed paranthesis marks the end of a list */
    if (*string == ')') {
      struct atto_token *closed_paren = allocate_token(NULL, ATTO_TOKEN_CLOSED_PAREN);
      if (token_list == NULL) {
        token_list = closed_paren;
        last_token = closed_paren;
      } else {
        last_token->next = closed_paren;
        last_token       = closed_paren;
      }

      string++;
    }
  }

  return token_list;
}

/*
 *  recursively deallocates a token list
 */
void destroy_token_list(struct atto_token *head)
{
  struct atto_token *current = head;

  while (current) {
    struct atto_token *temp = current->next;

    /*  if the token also contains an allocated string, free that as well */
    if (current->token != NULL) {
      free(current->token);
    }
    free(current);

    current = temp;
  }
}

