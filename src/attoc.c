
/*
 *  attoc.c
 *  the Atto compiler
 *  https://github.com/deveah/atto
 */

#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <string.h>

#include "parser.h"
#include "lexer.h"
#include "hashtable.h"

/*  TODO: refactor this to be nice to the ANSI C standard */
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
  
  input_buffer = (char *)malloc(sizeof(char) * (file_length + 1));
  assert(input_buffer != NULL);

  /*  dump the contents into the buffer, and close the handle */
  fread(input_buffer, sizeof(char), file_length, in);
  fclose(in);

  input_buffer[file_length] = 0;

  /*  execute compilation */

  struct atto_token *token_list = atto_lex_string(input_buffer);

  free(input_buffer);
  
  struct atto_token *left = NULL;

  struct hashtable *symbol_table = allocate_hashtable(ATTO_SYMBOL_TABLE_BUCKET_COUNT);

  struct atto_ast_node *root = atto_parse_token_list(token_list, &left, symbol_table, 2);

  destroy_token_list(token_list);

  pretty_print_ast(root, 0);

  struct atto_namespace *namespace = parse_namespace(root);
  namespace->symbol_table = symbol_table;

  destroy_ast(root);

  pretty_print_namespace(namespace);

  destroy_namespace(namespace);

  return 0;
}

