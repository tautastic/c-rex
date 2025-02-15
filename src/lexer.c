#include "lexer.h"

bool is_special(const rune c) {
    return (c >= 33 && c <= 47) || (c >= 58 && c <= 64) || (c >= 91 && c <= 96) || (c >= 123 && c <= 125);
}

const char *peek(const char *pattern, const size_t *pos, const size_t lookahead) {
    if (!pattern || !pos) {
        return NULL;
    }

    if (utf8size(pattern) < lookahead + *pos) {
        return NULL;
    }

    const char *current = pattern + *pos;

    rune result;
    for (size_t i = 0; i < lookahead; i++) {
        if (*current == '\0') {
            return NULL;
        }
        current = utf8codepoint(current, &result);
    }

    return current;
}

bool match(const char *pattern, size_t *pos, const char *expected) {
    if (!pos || !expected) {
        return false;
    }

    if (utf8str(pattern, expected) != pattern + *pos) {
        return false;
    }

    *pos += utf8codepointcalcsize(expected);
    return true;
}

const char *next(const char *pattern, size_t *pos, const size_t distance) {
    if (!pattern || !pos) {
        return NULL;
    }

    const char *current = NULL;
    for (size_t i = 0; i <= distance; i++) {
        current = peek(pattern, pos, 0);
        match(pattern, pos, current);
    }

    return current;
}

void strip_space(const char *pattern, size_t *pos) {
    if (!pattern || !pos) {
        return;
    }

    while (*peek(pattern, pos, 0) == ' ') {
        next(pattern, pos, 0);
    }
}

Node *root(const char *pattern, size_t *pos) {
    Node *node = create_node("<Root>");
    add_child(node, expr(pattern, pos));

    return node;
}

Node *expr(const char *pattern, size_t *pos) {
    Node *node = create_node("<Expr>");
    add_child(node, branch(pattern, pos));

    while (match(pattern, pos, "|")) {
        add_child(node, branch(pattern, pos));
    }

    return node;
}

Node *branch(const char *pattern, size_t *pos) {
    Node *node = create_node("<Branch>");
    add_child(node, piece(pattern, pos));

    return node;
}

Node *piece(const char *pattern, size_t *pos) {
    Node *node = create_node("<Piece>");
    const char *peek_ptr = peek(pattern, pos, 0);
    while (peek_ptr && *peek_ptr != '\0') {
        add_child(node, atom(pattern, pos));
        peek_ptr = peek(pattern, pos, 0);
    }

    return node;
}

//Node *assertion(const char *pattern, size_t *pos);

//Node *quantifier(const char *pattern, size_t *pos);

Node *atom(const char *pattern, size_t *pos) {
    Node *node = create_node("<Atom>");
    const char *peek_ptr = peek(pattern, pos, 0);
    if (peek_ptr && !is_special(*peek_ptr)) {
        add_child(node, literal(pattern, pos));
    }

    return node;
}

Node *literal(const char *pattern, size_t *pos) {
    Node *node = create_node("<Literal>");
    const char *next_ptr = next(pattern, pos, 0);
    if (next_ptr != NULL) {
        rune next_rune;
        utf8codepoint(next_ptr, &next_rune);
        Node *lit_val = create_node("");
        utf8catcodepoint(lit_val->label, next_rune, 15);
        add_child(node, lit_val);
    }

    return node;
}

Node *build_syntax_tree(const char *pattern) {
    size_t pos = 0;

    Node *res = root(pattern, &pos);
    return res;
}

