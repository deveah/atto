
/*
 *  attoc.c
 *  the Atto compiler :: https://deveah.github.io/atto
 */

#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <string.h>

#include "attoc.h"

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
    while (is_whitespace(*string)) {
      string++;
    }

    /*  identifiers start with a letter, and may contain any combination of
     *  letters, digits, and dashes */
    if (is_letter(*string)) {
      while (is_alnum(*string) || (*string == '-')) {
        temp_token[temp_token_index++] = *string++;
      }
      temp_token[temp_token_index] = 0;
      temp_token_index = 0;
      
      struct atto_token *identifier = allocate_token(temp_token, ATTO_TOKEN_IDENTIFIER);
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
 *  parses a token list and produces an abstract syntax tree based on it;
 *  a pointer to the first unconsumed token will be put in `left'
 */
struct atto_ast_node *atto_parse_token_list(struct atto_token *root, struct atto_token **left)
{
  struct atto_ast_node *ast_root = NULL;
  struct atto_ast_node *current_node = NULL;
  *left = root;

  while (*left) {
    if ((*left)->kind == ATTO_TOKEN_OPEN_PAREN) {
      struct atto_ast_node *temp = atto_parse_token_list((*left)->next, left);
      struct atto_ast_node *list = (struct atto_ast_node *)malloc(sizeof(struct atto_ast_node));
      assert(list != NULL);
      list->kind = ATTO_AST_NODE_LIST;
      list->container.list = temp;
      list->next = NULL;

      if (ast_root == NULL) {
        ast_root     = list;
        current_node = list;
      } else {
        current_node->next = list;
        current_node       = list;
      }

      continue;
    }

    if ((*left)->kind == ATTO_TOKEN_CLOSED_PAREN) {
      *left = (*left)->next;
      return ast_root;
    }

    if ((*left)->kind == ATTO_TOKEN_IDENTIFIER) {
      struct atto_ast_node *temp = (struct atto_ast_node *)malloc(sizeof(struct atto_ast_node));
      assert(temp != NULL);

      temp->kind = ATTO_AST_NODE_IDENTIFIER;
      temp->container.identifier = (char *)malloc(sizeof(char) * (strlen((*left)->token) + 1));
      strcpy(temp->container.identifier, (*left)->token);
      temp->next = NULL;

      if (ast_root == NULL) {
        ast_root     = temp;
        current_node = temp;
      } else {
        current_node->next = temp;
        current_node       = temp;
      }
    }

    *left = (*left)->next;
  }

  return ast_root;
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

/*
 *  recursively deallocates an AST
 */
void destroy_ast(struct atto_ast_node *root)
{
  struct atto_ast_node *current = root;

  while (current) {
    struct atto_ast_node *temp = current->next;

    /*  if the token is an identifier, also free its string storage */
    if (current->kind == ATTO_AST_NODE_IDENTIFIER) {
      free(current->container.identifier);
    }

    /*  if the token is a list, recurse and continue deallocating */
    if (current->kind == ATTO_AST_NODE_LIST) {
      destroy_ast(current->container.list);
    }

    free(current);

    current = temp;
  }
}

void pretty_print_ast(struct atto_ast_node *root, size_t level)
{
  struct atto_ast_node *current = root;
  size_t i;

  while (current) {
    if (current->kind == ATTO_AST_NODE_IDENTIFIER) {
      for (i = 0; i < level; i++) {
        printf("  ");
      }

      printf("ident: %s\n", current->container.identifier);
    }

    if (current->kind == ATTO_AST_NODE_LIST) {
      for (i = 0; i < level; i++) {
        printf("  ");
      }
      printf("list: (\n");
      pretty_print_ast(current->container.list, level + 1);

      for (i = 0; i < level; i++) {
        printf("  ");
      }
      printf(")\n");
    }
    
    current = current->next;
  }
}

int main(int argc, char **argv)
{
  FILE *in;
  size_t file_length;
  char *input_buffer;

  if (argc < 2) {
    printf("usage: %s [input]\n", argv[0]);
    return 1;
  }

  /*  open the file */
  in = fopen(argv[1], "r");
  if (in == NULL) {
    printf("fatal: unable to operate on file `%s'.\n", argv[1]);
    return 1;
  }

  /*  find out the file's length, and allocate a buffer to fit the whole
   *  contents of the file */
  fseek(in, 0, SEEK_END);
  file_length = ftell(in);
  fseek(in, 0, SEEK_SET);
  
  input_buffer = (char *)malloc(sizeof(char) * file_length);
  assert(input_buffer != NULL);

  /*  dump the contents into the buffer, and close the handle */
  fread(input_buffer, sizeof(char), file_length, in);
  fclose(in);

  /*  execute compilation */

  struct atto_token *token_list = atto_lex_string(input_buffer);
  
  struct atto_token *left = NULL;
  struct atto_ast_node *root = atto_parse_token_list(token_list, &left);

  pretty_print_ast(root, 0);

  destroy_token_list(token_list);

  return 0;
}

