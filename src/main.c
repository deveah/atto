
/*
 *  main.c
 *  Part of Atto :: https://deveah.github.io/atto
 */

#include <stdio.h>

#include "atto.h"

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
  (void) argc;
  (void) argv;

  struct atto_token *token_list = atto_lex_string("(a b (c (d e) f)) (g)");

  struct atto_token *left = NULL;
  struct atto_ast_node *root = atto_parse_token_list(token_list, &left);
  pretty_print_ast(root, 0);

  destroy_token_list(token_list);
  destroy_ast(root);

  return 0;
}

