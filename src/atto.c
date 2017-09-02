
/*
 *  atto.c
 *  part of Atto :: https://github.com/deveah/atto
 */

#include <stdio.h>
#include <stdlib.h>

#include <readline/readline.h>
#include <readline/history.h>

#include "state.h"
#include "parser.h"
#include "lexer.h"

#define COLOR_GREEN "\e[32m"
#define COLOR_RESET "\e[0m"

void evaluate_string(struct atto_state *a, char *str)
{
  struct atto_token *token_list  = atto_lex_string(str);
  struct atto_token *left = NULL;
  struct atto_ast_node *root = atto_parse_token_list(a, token_list, &left);
  struct atto_expression *e = NULL;

  if (root->kind == ATTO_AST_NODE_IDENTIFIER) {
    /*  search in global table for identifier */
    /*  display its value; if it's a thunk, execute it */
    /*  TODO */
  } else if (root->kind == ATTO_AST_NODE_LIST) {
    struct atto_ast_node *head = root->container.list;

    if (head->kind != ATTO_AST_NODE_IDENTIFIER) {
      printf("error: invalid syntax\n");

      destroy_ast(root);
      destroy_token_list(token_list);
      return;
    }

    if (strcmp(head->container.identifier, "define") == 0) {
      /*  compile */
      /*  save in global table */
      /*  TODO */
    } else {
      e = parse_expression(root);
      pretty_print_expression(e, 0);
      destroy_expression(e);
    }
  }

  destroy_token_list(token_list);
  destroy_ast(root);
}

int main(int argc, char **argv)
{
  (void) argc;
  (void) argv;

  char *line_buffer = NULL;

  printf("atto alpha -- https://github.com/deveah/atto\n");
  
  rl_variable_bind("blink-matching-paren", "on");

  struct atto_state *a = atto_allocate_state();

  while (1) {
    line_buffer = readline(COLOR_GREEN "atto" COLOR_RESET "> ");
    add_history(line_buffer);

    if (line_buffer == NULL) {
      break;
    }

    if (strcmp(line_buffer, "exit") == 0) {
      free(line_buffer);
      break;
    }

    evaluate_string(a, line_buffer);

    free(line_buffer);
  }

  atto_destroy_state(a);

  return 0;
}

