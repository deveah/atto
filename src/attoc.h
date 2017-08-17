
/*
 *  attoc.h
 *  the Atto Compiler :: https://deveah.github.io/atto
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
  uint32_t kind;

  struct atto_token *next;
};

struct atto_ast_node {
  #define ATTO_AST_NODE_IDENTIFIER  0
  #define ATTO_AST_NODE_NUMBER      1
  #define ATTO_AST_NODE_LIST        2
  uint32_t kind;

  union {
    char *identifier;
    double number;
    struct atto_ast_node *list;
  } container;

  struct atto_ast_node *next;
};

struct atto_define_form {
  char *identifier;
  struct atto_expression *body;
};

struct atto_lambda_expression {
  uint32_t number_of_parameters;
  char **parameter_names;
  struct atto_expression *body;
};

struct atto_if_expression {
  struct atto_expression *condition_expression;
  struct atto_expression *true_evaluation_expression;
  struct atto_expression *false_evaluation_expression;
};

struct atto_application_expression {
  char *identifier;
  uint32_t number_of_parameters;
  struct atto_expression **parameters;
};

struct atto_list_literal_expression {
  uint32_t number_of_elements;
  struct atto_expression *elements;
};

struct atto_expression {
  #define ATTO_EXPRESSION_KIND_NUMBER_LITERAL   0
  #define ATTO_EXPRESSION_KIND_SYMBOL_LITERAL   1
  #define ATTO_EXPRESSION_KIND_LIST_LITERAL     2
  #define ATTO_EXPRESSION_KIND_REFERENCE        3
  #define ATTO_EXPRESSION_KIND_LAMBDA           4
  #define ATTO_EXPRESSION_KIND_IF               5
  #define ATTO_EXPRESSION_KIND_APPLICATION      6
  uint32_t kind;

  union {
    double number_literal;
    uint64_t symbol_literal;
    struct atto_list_literal_expression *list_literal_expression;
    char *reference_identifier;
    struct atto_lambda_expression *lambda_expression;
    struct atto_if_expression *if_expression;
    struct atto_application_expression *application_expression;
  } container;
};

struct atto_token *atto_lex_string(const char *string);
struct atto_ast_node *atto_parse_token_list(struct atto_token *root,
  struct atto_token **left);
struct atto_expression *parse_expression(struct atto_ast_node *e);
struct atto_if_expression *parse_if_expression(struct atto_ast_node *head);
struct atto_application_expression *parse_application_expression(struct atto_ast_node *head);
struct atto_list_literal_expression *parse_list_literal_expression(struct atto_ast_node *head);
struct atto_lambda_expression *parse_lambda_expression(struct atto_ast_node *head);
struct atto_define_form *parse_define_form(struct atto_ast_node *head);
struct atto_define_form *transform_ast(struct atto_ast_node *root);
void destroy_token_list(struct atto_token *head);
void destroy_ast(struct atto_ast_node *root);

void puts_times(const char *what, size_t times);
void pretty_print_ast(struct atto_ast_node *root, size_t level);
void pretty_print_list_literal_expression(struct atto_list_literal_expression *e, int level);
void pretty_print_lambda_expression(struct atto_lambda_expression *e, int level);
void pretty_print_if_expression(struct atto_if_expression *e, int level);
void pretty_print_application_expression(struct atto_application_expression *e, int level);
void pretty_print_expression(struct atto_expression *e, int level);
void pretty_print_define_form(struct atto_define_form *d);
