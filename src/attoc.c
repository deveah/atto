
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
/*  TODO: refactor this to be nice to the ANSI C standard */
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
 *  parses a token list and produces an abstract syntax tree based on it;
 *  a pointer to the first unconsumed token will be put in `left'
 */
struct atto_ast_node *atto_parse_token_list(struct atto_token *root, struct atto_token **left)
{
  struct atto_ast_node *ast_root     = NULL;
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

    if ((*left)->kind == ATTO_TOKEN_NUMBER) {
      struct atto_ast_node *temp = NULL;
      char *endptr = NULL;

      temp = (struct atto_ast_node *)malloc(sizeof(struct atto_ast_node));
      assert(temp != NULL);

      temp->kind = ATTO_AST_NODE_NUMBER;
      temp->container.number = strtod((*left)->token, &endptr);
      temp->next = NULL;

      if ((*left)->token == endptr) {
        printf("syntax error: unable to parse number literal\n");
        return NULL;
      }

      if (ast_root == NULL) {
        ast_root     = temp;
        current_node = temp;
      } else {
        current_node->next = temp;
        current_node       = temp;
      }
    }

    if ((*left)->kind == ATTO_TOKEN_SYMBOL) {
      /*  XXX TODO */
    }

    *left = (*left)->next;
  }

  return ast_root;
}

/*
 *  parses an expression from its AST root node
 *
 *  <expression> ::=  <number_literal> | <symbol_literal> |
 *                    <variable_reference> | <application_expression> |
 *                    <lambda_expression> | <if_expression> |
 *                    <list_literal_expression>
 *
 *  <list_literal_expression> ::= list {<expression>}*
 */
struct atto_expression *parse_expression(struct atto_ast_node *e)
{
  struct atto_expression *expression = NULL;

  if (e == NULL) {
    printf("syntax error: expected expression\n");
    return NULL;
  }

  expression = (struct atto_expression *)malloc(sizeof(struct atto_expression));

  /*  an expression can be a number literal */
  if (e->kind == ATTO_AST_NODE_NUMBER) {
    expression->kind = ATTO_EXPRESSION_KIND_NUMBER_LITERAL;
    expression->container.number_literal = e->container.number;

    return expression;
  }

  /*  an expression can be an identifier, in which case it is considered to be
   *  a variable reference */
  if (e->kind == ATTO_AST_NODE_IDENTIFIER) {
    expression->kind = ATTO_EXPRESSION_KIND_REFERENCE;
    expression->container.reference_identifier = (char *)malloc(sizeof(char) * (strlen(e->container.identifier) + 1));
    strcpy(expression->container.reference_identifier, e->container.identifier);

    return expression;
  }

  /*  an expression can be a list, in which the contents of the list give the
   *  meaning of the expression */
  if (e->kind == ATTO_AST_NODE_LIST) {
    struct atto_ast_node *head = e->container.list;

    /*  if the first element of the list is an identifier, the expression can
     *  be either a function application, or, if the identifier is a keyword,
     *  the expression is a syntactic form */
    if (head->kind == ATTO_AST_NODE_IDENTIFIER) {

      /*  lambda expression */
      if (strcmp(head->container.identifier, "lambda") == 0) {
        expression->kind = ATTO_EXPRESSION_KIND_LAMBDA;
        expression->container.lambda_expression = parse_lambda_expression(head);

        return expression;
      }

      /*  if expression */
      if (strcmp(head->container.identifier, "if") == 0) {
        expression->kind = ATTO_EXPRESSION_KIND_IF;
        expression->container.if_expression = parse_if_expression(head);

        return expression;
      }

      /*  list literal expression */
      if (strcmp(head->container.identifier, "list") == 0) {
        expression->kind = ATTO_EXPRESSION_KIND_LIST_LITERAL;
        expression->container.list_literal_expression = parse_list_literal_expression(head);

        return expression;
      }

      /*  if the identifier is not a keyword, then parse it as a function
       *  application */
      expression->kind = ATTO_EXPRESSION_KIND_APPLICATION;
      expression->container.application_expression = parse_application_expression(head);

      return expression;
    }

    /*  if the first element of the list is a list, then it must represent
     *  an anonymous function application */
    if (head->kind == ATTO_AST_NODE_LIST) {
      struct atto_ast_node *child = head->container.list;
      struct atto_lambda_expression *child_lambda_expression = NULL;

      /*  the list must represent a valid lambda expression */
      if ((child->kind != ATTO_AST_NODE_IDENTIFIER) ||
          (strcmp(child->container.identifier, "lambda") != 0)) {
        printf("syntax error: invalid expression in anonymous function application\n");
        return NULL;
      }

      child_lambda_expression = parse_lambda_expression(child);
      if (child_lambda_expression == NULL) {
        printf("syntax error: invalid expression in anonymous function application\n");
        return NULL;
      }

      expression->kind = ATTO_EXPRESSION_KIND_LAMBDA;
      expression->container.lambda_expression = child_lambda_expression;

      return expression;
    }

    /*  if the first element of the list is a number, then raise a syntax error;
     *  list literals must be expresses through the `list' syntactic form */ 
    if (head->kind == ATTO_AST_NODE_NUMBER) {
      printf("syntax error: invalid expression; for quoted lists use the `list' form\n");
      return NULL;
    }
  }

  /*  this point should be unreachable */
  return NULL;
}

/*
 *  parse an if expression from its AST root node
 *
 *  <if_expression> ::= if <expression> <expression> <expression>
 */
struct atto_if_expression *parse_if_expression(struct atto_ast_node *head)
{
  struct atto_ast_node *condition    = NULL;
  struct atto_ast_node *true_branch  = NULL;
  struct atto_ast_node *false_branch = NULL;

  struct atto_expression *condition_expression        = NULL;
  struct atto_expression *true_evaluation_expression  = NULL;
  struct atto_expression *false_evaluation_expression = NULL;

  struct atto_if_expression *if_expression = NULL;

  condition = head->next;
  if (condition == NULL) {
    printf("syntax error: expected condition expression in `if' expression\n");
    return NULL;
  }

  true_branch = head->next->next;
  if (true_branch == NULL) {
    printf("syntax error: expected true evaluation expression in `if' expression\n");
    return NULL;
  }

  false_branch = head->next->next->next;
  if (false_branch == NULL) {
    printf("syntax error: expected false evaluation expression in `if' expression\n");
    return NULL;
  }

  condition_expression        = parse_expression(condition);
  true_evaluation_expression  = parse_expression(true_branch);
  false_evaluation_expression = parse_expression(false_branch);

  if_expression = (struct atto_if_expression *)malloc(sizeof(struct atto_if_expression));

  if_expression->condition_expression        = condition_expression;
  if_expression->true_evaluation_expression  = true_evaluation_expression;
  if_expression->false_evaluation_expression = false_evaluation_expression;

  return if_expression;
}

/*
 *  parse an application expression from its AST root node
 *
 *  <application_expression> ::= <variable_reference> {<expression>}+
 */
struct atto_application_expression *parse_application_expression(struct atto_ast_node *head)
{
  struct atto_ast_node *reference  = head;
  struct atto_ast_node *current    = NULL;

  char *identifier = NULL;

  uint32_t number_of_parameters    = 0;
  uint32_t current_parameter_index = 0;

  struct atto_application_expression *application_expression = NULL;

  if ((reference == NULL) ||
      (reference->kind != ATTO_AST_NODE_IDENTIFIER)) {
    printf("syntax error: expected reference in application expression\n");
    return NULL;
  }

  application_expression = (struct atto_application_expression *)malloc(sizeof(struct atto_application_expression));
  assert(application_expression != NULL);

  identifier = (char *)malloc(sizeof(char) * (strlen(head->container.identifier) + 1));
  assert(identifier != NULL);
  strcpy(identifier, head->container.identifier);
  application_expression->identifier = identifier;

  /*  count the number of parameters in order to know the size of the parameter
   *  array to be allocated */
  current = head->next;
  while (current) {
    number_of_parameters++;
    current = current->next;
  }

  application_expression->number_of_parameters = number_of_parameters;
  application_expression->parameters = (struct atto_expression **)malloc(sizeof(struct atto_expression *) * number_of_parameters);

  current = head->next;
  while (current) {
    application_expression->parameters[current_parameter_index] = parse_expression(current);

    current_parameter_index++;
    current = current->next;
  }

  return application_expression;
}

/*  XXX TODO */
struct atto_list_literal_expression *parse_list_literal_expression(struct atto_ast_node *head)
{
  return NULL;
}

/*
 *  parse a lambda expression from its AST root node
 *
 *  <lambda_expression> ::= lambda {<identifier>}+ <expression>
 */
struct atto_lambda_expression *parse_lambda_expression(struct atto_ast_node *head)
{
  struct atto_ast_node *parameter_list = NULL;
  struct atto_ast_node *body           = NULL;
  struct atto_ast_node *current        = NULL;

  struct atto_lambda_expression *lambda = NULL;
  
  uint32_t number_of_parameters    = 0;
  uint32_t current_parameter_index = 0;

  struct atto_expression *body_expression = NULL;

  /*  the first argument of the lambda form is a list of parameter
   *  identifiers */
  parameter_list = head->next;
  if ((parameter_list == NULL) ||
      (parameter_list->kind != ATTO_AST_NODE_LIST)) {
    printf("syntax error: expected parameter list in `lambda' form\n");
    return NULL;
  }

  /*  the second argument of the lambda form is an expression which is
   *  being defined */
  body = head->next->next;
  if ((body == NULL) ||
      (body->kind != ATTO_AST_NODE_LIST)) {
    printf("syntax error: expected expression in `lambda' form\n");
    return NULL;
  }

  /*  every element in the parameter list must be an identifier; furthermore,
   *  a counter is used to predict the size of the parameter array in the
   *  lambda structure */
  current = parameter_list->container.list;
  while (current) {
    if (current->kind != ATTO_AST_NODE_IDENTIFIER) {
      printf("syntax error: expected identifier in parameter list of `lambda' form\n");
      return NULL;
    }

    number_of_parameters++;
    current = current->next;
  }

  /*  the body of the lambda form must be a valid expression */
  body_expression = parse_expression(body);
  if (body_expression == NULL) {
    printf("syntax error: unable to parse `lambda' form; giving up\n");
    return NULL;
  }

  /*  now that we've assured the correctness of the lambda form, we begin
   *  transforming the raw AST data into a lambda structure */
  lambda = (struct atto_lambda_expression *)malloc(sizeof(struct atto_lambda_expression));
  lambda->number_of_parameters = number_of_parameters;
  lambda->parameter_names = (char **)malloc(sizeof(char *) * number_of_parameters);
  lambda->body = body_expression;

  /*  copy the identifiers into the list of parameter names of the lambda
   *  structure */
  current = parameter_list->container.list;
  while (current) {
    lambda->parameter_names[current_parameter_index] = (char *)malloc(sizeof(char) * (strlen(current->container.identifier) + 1));
    strcpy(lambda->parameter_names[current_parameter_index], current->container.identifier);

    current_parameter_index++;
    current = current->next;
  }

  return lambda;
}

/*
 *  parses a definition from its AST root node
 *
 *  <definition> ::= define <identifier> <expression>
 */
struct atto_definition *parse_definition(struct atto_ast_node *head)
{
  struct atto_ast_node *identifier = NULL;
  struct atto_ast_node *body       = NULL;
  
  struct atto_definition *definition = NULL;

  struct atto_expression *expression = NULL;

  /*  the first argument of the `define' form must be an identifier */
  identifier = head->next;
  if ((identifier == NULL) ||
      (identifier->kind != ATTO_AST_NODE_IDENTIFIER)) {
    printf("syntax error: expected identifier in `define' form\n");
    return NULL;
  }

  /*  the body of the `define' form must be a valid expression */
  body = head->next->next;
  if (body == NULL) {
    printf("syntax error: expected body expression in `define' form\n");
    return NULL;
  }

  expression = parse_expression(body);
  if (expression == NULL) {
    printf("syntax error: unable to parse `define' form; giving up\n");
    return NULL;
  }

  definition = (struct atto_definition *)malloc(sizeof(struct atto_definition));
  definition->identifier = (char *)malloc(sizeof(char) * (strlen(identifier->container.identifier) + 1));
  strcpy(definition->identifier, identifier->container.identifier);
  definition->body = expression;

  return definition;
}

/*
 *  parses a namespace (a list of definitions) from its AST root node;
 *  please note that the given root node can be part of a list of
 *  nodes
 */
struct atto_namespace *parse_namespace(struct atto_ast_node *root)
{
  struct atto_ast_node *current = root;

  uint32_t number_of_definitions    = 0;
  uint32_t current_definition_index = 0;

  struct atto_namespace *namespace = NULL;

  while (current) {
    struct atto_ast_node *body = NULL;

    if (current->kind != ATTO_AST_NODE_LIST) {
      printf("syntax error: only definitions are allowed in a namespace\n");
      return NULL;
    }

    body = current->container.list;

    if (body->kind != ATTO_AST_NODE_IDENTIFIER) {
      printf("syntax error: only definitions are allowed in a namespace\n");
      return NULL;
    }

    if (strcmp(body->container.identifier, "define") != 0) {
      printf("syntax error: only definitions are allowed in a namespace\n");
      return NULL;
    }

    number_of_definitions++;
    current = current->next;
  }

  namespace = (struct atto_namespace *)malloc(sizeof(struct atto_namespace));
  namespace->number_of_definitions = number_of_definitions;
  namespace->definitions = (struct atto_definition **)malloc(sizeof(struct atto_definition *) * number_of_definitions);

  current = root;
  while (current) {
    namespace->definitions[current_definition_index] = parse_definition(current->container.list);

    current_definition_index++;
    current = current->next;
  }

  return namespace;
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

void puts_times(const char *what, size_t times)
{
  size_t i;

  for (i = 0; i < times; i++) {
    printf("%s", what);
  }
}

void pretty_print_ast(struct atto_ast_node *root, size_t level)
{
  struct atto_ast_node *current = root;

  while (current) {
    if (current->kind == ATTO_AST_NODE_IDENTIFIER) {
      puts_times("  ", level);
      printf("ident: %s\n", current->container.identifier);
    }

    if (current->kind == ATTO_AST_NODE_LIST) {
      puts_times("  ", level);
      printf("list: (\n");
      pretty_print_ast(current->container.list, level + 1);

      puts_times("  ", level);
      printf(")\n");
    }
    
    current = current->next;
  }
}

void pretty_print_list_literal_expression(struct atto_list_literal_expression *e, int level)
{
  uint32_t i;

  for (i = 0; i < e->number_of_elements; i++) {
    puts_times("  ", level);
    pretty_print_expression(e->elements[i], level + 1);
  }
}

void pretty_print_lambda_expression(struct atto_lambda_expression *e, int level)
{
  uint32_t i;

  puts_times("  ", level);
  printf("number of parameters: %i\n", e->number_of_parameters);

  puts_times("  ", level);
  printf("parameter names: ");

  for (i = 0; i < e->number_of_parameters; i++) {
    printf("%s", e->parameter_names[i]);

    if (i < e->number_of_parameters - 1) {
      printf(", ");
    }
  }

  printf("\n");
  puts_times("  ", level);
  printf("body:\n");
  pretty_print_expression(e->body, level + 1);
}

void pretty_print_if_expression(struct atto_if_expression *e, int level)
{
  puts_times("  ", level);
  printf("condition expression:\n");
  pretty_print_expression(e->condition_expression, level + 1);

  puts_times("  ", level);
  printf("true evaluation expression:\n");
  pretty_print_expression(e->true_evaluation_expression, level + 1);

  puts_times("  ", level);
  printf("false evaluation expression:\n");
  pretty_print_expression(e->false_evaluation_expression, level + 1);
}

void pretty_print_application_expression(struct atto_application_expression *e, int level)
{
  uint32_t i;

  puts_times("  ", level);
  printf("function identifier: %s\n", e->identifier);

  puts_times("  ", level);
  printf("number of parameters: %i\n", e->number_of_parameters);

  puts_times("  ", level);
  printf("parameters:\n");

  for (i = 0; i < e->number_of_parameters; i++) {
    pretty_print_expression(e->parameters[i], level + 1);
  }
}

void pretty_print_expression(struct atto_expression *e, int level)
{
  int i;
  for (i = 0; i < level; i++) {
    printf("  ");
  }

  switch (e->kind) {
  
  case ATTO_EXPRESSION_KIND_NUMBER_LITERAL:
    printf("number literal: %lf\n", e->container.number_literal);
    return;

  case ATTO_EXPRESSION_KIND_SYMBOL_LITERAL:
    printf("symbol literal: %lu\n", e->container.symbol_literal);
    return;

  case ATTO_EXPRESSION_KIND_LIST_LITERAL:
    printf("list literal:\n");
    pretty_print_list_literal_expression(e->container.list_literal_expression, level + 1);
    return;

  case ATTO_EXPRESSION_KIND_REFERENCE:
    printf("variable reference: %s\n", e->container.reference_identifier);
    return;

  case ATTO_EXPRESSION_KIND_LAMBDA:
    printf("lambda expression:\n");
    pretty_print_lambda_expression(e->container.lambda_expression, level + 1);
    return;

  case ATTO_EXPRESSION_KIND_IF:
    printf("if expression:\n");
    pretty_print_if_expression(e->container.if_expression, level + 1);
    return;

  case ATTO_EXPRESSION_KIND_APPLICATION:
    printf("application expression:\n");
    pretty_print_application_expression(e->container.application_expression, level + 1);
    return;

  default:
    printf("unknown expression kind: %i\n", e->kind);

  }
}

void pretty_print_definition(struct atto_definition *d)
{
  printf("definition:\n");
  printf("  identifier: %s\n", d->identifier);
  printf("  body:\n");
  pretty_print_expression(d->body, 2);
  printf("\n");
}

void pretty_print_namespace(struct atto_namespace *n)
{
  size_t i;

  printf("number of definitions: %i\n", n->number_of_definitions);

  for (i = 0; i < n->number_of_definitions; i++) {
    pretty_print_definition(n->definitions[i]);
  }
}

void destroy_expression(struct atto_expression *e)
{
  uint32_t i;

  switch (e->kind) {
  
  case ATTO_EXPRESSION_KIND_LIST_LITERAL: {
    struct atto_list_literal_expression *lle = e->container.list_literal_expression;
    for (i = 0; i < lle->number_of_elements; i++) {
      destroy_expression(lle->elements[i]);
    }
    free(lle);
    break;
  }

  case ATTO_EXPRESSION_KIND_REFERENCE: {
    char *reference_identifier = e->container.reference_identifier;
    free(reference_identifier);
    break;
  }

  case ATTO_EXPRESSION_KIND_LAMBDA: {
    struct atto_lambda_expression *le = e->container.lambda_expression;
    for (i = 0; i < le->number_of_parameters; i++) {
      free(le->parameter_names[i]);
    }
    free(le->parameter_names);
    destroy_expression(le->body);
    free(le);
    break;
  }

  case ATTO_EXPRESSION_KIND_IF: {
    struct atto_if_expression *ie = e->container.if_expression;
    destroy_expression(ie->condition_expression);
    destroy_expression(ie->true_evaluation_expression);
    destroy_expression(ie->false_evaluation_expression);
    free(ie);
    break;
  }

  case ATTO_EXPRESSION_KIND_APPLICATION: {
    struct atto_application_expression *ae = e->container.application_expression;
    free(ae->identifier);
    for (i = 0; i < ae->number_of_parameters; i++) {
      destroy_expression(ae->parameters[i]);
    }
    free(ae->parameters);
    free(ae);
    break;
  }

  default:
    break;
  }

  free(e);
}

void destroy_namespace(struct atto_namespace *n)
{
  uint32_t i;

  for (i = 0; i < n->number_of_definitions; i++) {
    free(n->definitions[i]->identifier);
    destroy_expression(n->definitions[i]->body);
    free(n->definitions[i]);
  }

  free(n->definitions);
  free(n);
}

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

  struct atto_ast_node *root = atto_parse_token_list(token_list, &left);

  destroy_token_list(token_list);

  pretty_print_ast(root, 0);

  struct atto_namespace *namespace = parse_namespace(root);

  destroy_ast(root);

  pretty_print_namespace(namespace);

  destroy_namespace(namespace);

  return 0;
}

