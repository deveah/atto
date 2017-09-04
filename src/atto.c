
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
#include "compiler.h"

#define COLOR_GREEN  "\e[32m"
#define COLOR_YELLOW "\e[33m"
#define COLOR_RESET  "\e[0m"

size_t result_count = 0;

void print_result(struct atto_state *a)
{
  struct atto_object *o = NULL;

  printf(COLOR_YELLOW "[%lu] " COLOR_RESET, result_count++);

  o = a->vm_state->data_stack[a->vm_state->data_stack_size - 1];

  switch (o->kind) {
  
  case ATTO_OBJECT_KIND_NULL:
    printf("()");
    break;

  case ATTO_OBJECT_KIND_NUMBER:
    printf("%e", o->container.number);
    break;

  case ATTO_OBJECT_KIND_SYMBOL:
    printf("%s", a->symbol_names[o->container.symbol]);
    break;

  case ATTO_OBJECT_KIND_LIST:
    printf("()");
    break;

  case ATTO_OBJECT_KIND_LAMBDA:
    printf("lambda#%lu", o->container.instruction_stream_index);
    break;

  case ATTO_OBJECT_KIND_THUNK:
    printf("thunk#%lu", o->container.instruction_stream_index);
    break;

  }

  printf("\n");
}

void evaluate_string(struct atto_state *a, char *str)
{
  struct atto_token *token_list  = atto_lex_string(str);
  struct atto_token *left = NULL;
  struct atto_ast_node *root = atto_parse_token_list(a, token_list, &left);
  struct atto_expression *e = NULL;

  struct atto_instruction_stream *is = (struct atto_instruction_stream *)malloc(sizeof(struct atto_instruction_stream));
  is->length = 0;
  is->allocated_length = 32;
  is->stream = (uint8_t *)malloc(sizeof(uint8_t) * is->allocated_length);

  if (root->kind == ATTO_AST_NODE_IDENTIFIER) {
    struct atto_environment_object *current = a->global_environment->head;

    while (current) {
      if (strcmp(current->name, root->container.identifier) == 0) {
        struct atto_object *o = atto_get_object(a, current);
        
        if (o->kind == ATTO_OBJECT_KIND_THUNK) {
          evaluate_thunk(a->vm_state, o);
        }

        print_result(a);
        break;
      }

      current = current->next;
    }
  } else if (root->kind == ATTO_AST_NODE_LIST) {
    struct atto_ast_node *head = root->container.list;

    if (head->kind != ATTO_AST_NODE_IDENTIFIER) {
      printf("error: invalid syntax\n");

      destroy_ast(root);
      destroy_token_list(token_list);
      return;
    }

    if (strcmp(head->container.identifier, "define") == 0) {
      struct atto_definition *definition = parse_definition(head);
      /*pretty_print_definition(definition);*/

      compile_definition(a, definition);
      pretty_print_stack(a->vm_state);

      destroy_expression(definition->body);
      free(definition->identifier);
      free(definition);
    } else {
      e = parse_expression(root);
      /*pretty_print_expression(e, 0);
      printf("-------------------------------------------------\n");*/
      compile_expression(a, a->global_environment, is, e);

      /*printf("-------------------------------------------------\n");
      pretty_print_instruction_stream(is);*/

      a->vm_state->instruction_streams[a->vm_state->number_of_instruction_streams] = is;
      a->vm_state->number_of_instruction_streams += 1;

      a->vm_state->current_instruction_stream_index = a->vm_state->number_of_instruction_streams - 1;
      a->vm_state->current_instruction_offset = 0;
      atto_run_vm(a->vm_state);

      print_result(a);

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
  a->vm_state = atto_allocate_vm_state();

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

