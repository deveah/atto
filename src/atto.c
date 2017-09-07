
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

static void pretty_print_list(struct atto_state *a, size_t index);

void pretty_print_object(struct atto_state *a, size_t index)
{
  switch (a->vm_state->heap[index].kind) {
  
  case ATTO_OBJECT_KIND_NULL:
    printf("()");
    break;

  case ATTO_OBJECT_KIND_NUMBER:
    printf("%e", a->vm_state->heap[index].container.number);
    break;

  case ATTO_OBJECT_KIND_SYMBOL:
    printf("%s", a->symbol_names[a->vm_state->heap[index].container.symbol]);
    break;

  case ATTO_OBJECT_KIND_LIST:
    pretty_print_list(a, index);
    break;

  case ATTO_OBJECT_KIND_LAMBDA:
    printf("lambda#%lu", a->vm_state->heap[index].container.instruction_stream_index);
    break;

  case ATTO_OBJECT_KIND_THUNK:
    printf("thunk#%lu", a->vm_state->heap[index].container.instruction_stream_index);
    break;

  }
}

static void pretty_print_list(struct atto_state *a, size_t index)
{
  printf("(");
  pretty_print_object(a, a->vm_state->heap[index].container.list.car);

  if (a->vm_state->heap[a->vm_state->heap[index].container.list.cdr].kind != ATTO_OBJECT_KIND_NULL) {
    printf(" ");
    pretty_print_list(a, a->vm_state->heap[index].container.list.cdr);
  }

  printf(")");
}

static void pretty_print_result(struct atto_state *a, size_t index)
{
  printf(COLOR_YELLOW "[%lu] " COLOR_RESET, result_count++);
  pretty_print_object(a, index);
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
  is->stream = (struct atto_instruction *)malloc(sizeof(struct atto_instruction) * is->allocated_length);

  if (root->kind == ATTO_AST_NODE_IDENTIFIER) {
    struct atto_environment_object *current = a->global_environment->head;

    while (current) {
      if (strcmp(current->name, root->container.identifier) == 0) {
        size_t o = atto_get_object(a, current);
        
        if (a->vm_state->heap[o].kind == ATTO_OBJECT_KIND_THUNK) {
          evaluate_thunk(a->vm_state, o);
        }

        pretty_print_result(a, o);
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
      /*pretty_print_stack(a->vm_state);*/
      
      pretty_print_result(a, a->vm_state->data_stack[a->vm_state->data_stack_size - 1]);

      destroy_expression(definition->body);
      free(definition->identifier);
      free(definition);
    } else {
      e = parse_expression(root);
      /*pretty_print_expression(e, 0);
      printf("-------------------------------------------------\n");*/
      compile_expression(a, a->global_environment, is, e);

      a->vm_state->instruction_streams[a->vm_state->number_of_instruction_streams] = is;
      a->vm_state->number_of_instruction_streams += 1;

      a->vm_state->current_instruction_stream_index = a->vm_state->number_of_instruction_streams - 1;
      a->vm_state->current_instruction_offset = 0;
      atto_run_vm(a->vm_state);

      pretty_print_result(a, a->vm_state->data_stack[a->vm_state->data_stack_size - 1]);

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
  printf("type " COLOR_YELLOW "-help" COLOR_RESET " in case of emergency\n");
  
  rl_variable_bind("blink-matching-paren", "on");

  struct atto_state *a = atto_allocate_state();
  a->vm_state = atto_allocate_vm_state();

  while (1) {
    line_buffer = readline(COLOR_GREEN "atto" COLOR_RESET "> ");
    add_history(line_buffer);

    if (line_buffer == NULL) {
      break;
    }

    if (strcmp(line_buffer, "-exit") == 0) {
      free(line_buffer);
      break;
    }

    if (strcmp(line_buffer, "-help") == 0) {
      printf("REPL commands:\n");
      printf(COLOR_YELLOW "  -exit\n" COLOR_RESET);
      printf(COLOR_YELLOW "  -stack\t" COLOR_RESET "displays the stack\n");
      printf(COLOR_YELLOW "  -env\t\t" COLOR_RESET "displays the global environment\n");
      printf(COLOR_YELLOW "  -verbose-on\n" COLOR_RESET);
      printf(COLOR_YELLOW "  -verbose-off\n" COLOR_RESET);
      printf(COLOR_YELLOW "  -heap-usage\n" COLOR_RESET);
      free(line_buffer);
      continue;
    }

    if (strcmp(line_buffer, "-stack") == 0) {
      pretty_print_stack(a->vm_state);
      free(line_buffer);
      continue;
    }

    if (strcmp(line_buffer, "-env") == 0) {
      pretty_print_environment(a->global_environment);
      free(line_buffer);
      continue;
    }

    if (strcmp(line_buffer, "-verbose-on") == 0) {
      a->vm_state->flags |= ATTO_VM_FLAG_VERBOSE;
      free(line_buffer);
      continue;
    }

    if (strcmp(line_buffer, "-verbose-off") == 0) {
      a->vm_state->flags &= ~(ATTO_VM_FLAG_VERBOSE);
      free(line_buffer);
      continue;
    }

    if (strcmp(line_buffer, "-heap-usage") == 0) {
      pretty_print_heap_usage(a->vm_state);
      free(line_buffer);
      continue;
    }

    evaluate_string(a, line_buffer);

    free(line_buffer);
  }

  atto_destroy_state(a);

  return 0;
}

