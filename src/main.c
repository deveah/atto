
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

  /*
   *  parser/lexer testing

  struct atto_token *token_list = atto_lex_string("(a b (c (d e) f)) (g)");

  struct atto_token *left = NULL;
  struct atto_ast_node *root = atto_parse_token_list(token_list, &left);
  pretty_print_ast(root, 0);

  destroy_token_list(token_list);
  destroy_ast(root);

   */

  /*  allocate a state with one function */
  struct atto_vm_state *A = allocate_state(1);

  /*
   *  allocate a function with:
   *  1 argument
   *  1 constant, and
   *  3 instructions
   */
  struct atto_vm_function *f1 = allocate_function(1, 1, 3);
  f1->constants[0] = 1;
  f1->instructions[0] = 0x01010000; /* r1 = constant #0 */
  f1->instructions[1] = 0x02020001; /* r2 = r0 + r1 */
  f1->instructions[2] = 0xf0020000; /* return r2 */

  A->functions[0] = f1;

  /*  r0 = 5 (first argument of the function) */
  A->registers[0] = 5;

  /*  initialize the state of the machine */
  A->current_function = 0;
  A->current_instruction = 0;

  do {
    perform_step(A);
  } while(A->current_function != 0xffffffff);

  return 0;
}

