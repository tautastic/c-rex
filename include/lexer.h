#pragma once

#include <stdbool.h>
#include "ast.h"
#include "utf8.h"

const char *peek(const char *pattern, const size_t *pos, size_t lookahead);

bool match(const char *pattern, size_t *pos, const char *expected);

const char *next(const char *pattern, size_t *pos, size_t distance);

void strip_space(const char *pattern, size_t *pos);

Node *root(const char *pattern, size_t *pos);

Node *expr(const char *pattern, size_t *pos);

Node *branch(const char *pattern, size_t *pos);

Node *piece(const char *pattern, size_t *pos);

Node *assertion(const char *pattern, size_t *pos);

Node *quantifier(const char *pattern, size_t *pos);

Node *atom(const char *pattern, size_t *pos);

Node *atom_escape(const char *pattern, size_t *pos);

Node *hex_sequence(const char *pattern, size_t *pos);

Node *unicode_sequence(const char *pattern, size_t *pos);

Node *char_class(const char *pattern, size_t *pos);

Node *class_range(const char *pattern, size_t *pos);

Node *class_atom(const char *pattern, size_t *pos);

Node *literal(const char *pattern, size_t *pos);

Node *decimal_digits(const char *pattern, size_t *pos);

Node *build_syntax_tree(const char *pattern);

