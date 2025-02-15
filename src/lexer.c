#include "lexer.h"

bool is_special(const char c) {
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
    Node *branch_node = branch(pattern, pos);
    add_child(node, branch_node);

    if (match(pattern, pos, "|")) {
        add_child(node, branch(pattern, pos));
    } else {
        add_child(branch_node, piece(pattern, pos));
    }

    return node;
}

Node *branch(const char *pattern, size_t *pos) {
    Node *node = create_node("<Branch>");

    const char *peek_ptr = peek(pattern, pos, 0);
    if (peek_ptr && *peek_ptr != '\0') {
        add_child(node, piece(pattern, pos));
    }

    return node;
}

Node *piece(const char *pattern, size_t *pos) {
    Node *atom_node = atom(pattern, pos);
    if (!atom_node) {
        return NULL;
    }
    Node *node = create_node("<Piece>");
    add_child(node, atom_node);

    return node;
}

//Node *assertion(const char *pattern, size_t *pos);

//Node *quantifier(const char *pattern, size_t *pos);

Node *atom(const char *pattern, size_t *pos) {
    Node *node = create_node("<Atom>");
    const char *peek_ptr = peek(pattern, pos, 0);
    if (peek_ptr) {
        switch (*peek_ptr) {
            case '.': {
                match(pattern, pos, ".");
                add_child(node, create_node("<Dot>"));
                break;
            }
            case '\\': {
                match(pattern, pos, "\\");
                //add_child(node, atom_escape(pattern, pos));
                break;
            }
            case '[': {
                //add_child(node, char_class(pattern, pos));
                break;
            }
            case '(': {
                match(pattern, pos, "(");
                Node *expr_node = expr(pattern, pos);
                match(pattern, pos, ")");
                add_child(node, expr_node);
                break;
            }
            default: {
                if (is_special(*peek_ptr)) {
                    free_node(node);
                    return NULL;
                }
                add_child(node, literal(pattern, pos));
                break;
            }
        }
    }

    return node;
}

Node *literal(const char *pattern, size_t *pos) {
    Node *node = create_node("<Literal>");
    const char *next_ptr = next(pattern, pos, 0);
    rune peek_rune;
    utf8codepoint(next_ptr, &peek_rune);

    Node *lit_val = create_node("");
    utf8catcodepoint(lit_val->label, peek_rune, 15);
    add_child(node, lit_val);

    return node;
}

Node *build_syntax_tree(const char *pattern) {
    size_t pos = 0;

    Node *res = root(pattern, &pos);
    return res;
}

