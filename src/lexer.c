#include "lexer.h"

bool is_special(const char c) {
    return (c >= 33 && c <= 47) || (c >= 58 && c <= 64) || (c >= 91 && c <= 96) || (c >= 123 && c <= 125);
}

bool is_digit(const char c) {
    return '0' <= c && c <= '9';
}

bool is_upercase_letter(const char c) {
    return 'A' <= c && c <= 'Z';
}

bool is_lowercase_letter(const char c) {
    return 'a' <= c && c <= 'z';
}

bool is_hex(const char c) {
    return ('0' <= c && c <= '9') || ('a' <= c && c <= 'f') || ('A' <= c && c <= 'F');
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
    const char *peek_ptr = peek(pattern, pos, 0);

    if (!peek_ptr || *peek_ptr != *expected) {
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
    while (peek_ptr && *peek_ptr != '\0' && *peek_ptr != ')' && *peek_ptr != '}') {
        add_child(node, piece(pattern, pos));
        peek_ptr = peek(pattern, pos, 0);
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
    add_child(node, quantifier(pattern, pos));

    return node;
}

//Node *assertion(const char *pattern, size_t *pos);

Node *quantifier(const char *pattern, size_t *pos) {
    const char *peek_ptr = peek(pattern, pos, 0);
    if (!peek_ptr) {
        return NULL;
    }

    Node *lower = NULL;
    Node *upper = NULL;

    switch (*peek_ptr) {
        case '?': {
            match(pattern, pos, "?");
            lower = create_node("0");
            upper = create_node("1");
            break;
        }
        case '*': {
            match(pattern, pos, "*");
            lower = create_node("0");
            upper = create_node("-1");
            break;
        }
        case '+': {
            match(pattern, pos, "+");
            lower = create_node("1");
            upper = create_node("-1");
            break;
        }
        case '{': {
            match(pattern, pos, "{");
            strip_space(pattern, pos);
            Node *lower_part = decimal_digits(pattern, pos);
            strip_space(pattern, pos);
            if (match(pattern, pos, ",")) {
                strip_space(pattern, pos);
                Node *upper_part = decimal_digits(pattern, pos);
                strip_space(pattern, pos);
                if (!lower_part && !upper_part) {
                    free_node(lower_part);
                    free_node(upper_part);
                    return NULL;
                }
                lower = lower_part ? lower_part : create_node("0");
                upper = upper_part ? upper_part : create_node("-1");
            } else {
                if (!lower_part) {
                    return NULL;
                }
                upper = create_node(lower_part->label);
                lower = lower_part;
            }
            if (!match(pattern, pos, "}")) {
                free_node(lower);
                free_node(upper);
                return NULL;
            }
            const int lower_val = atoi(lower->label);
            const int upper_val = atoi(upper->label);
            if (lower_val < 0) {
                free_node(lower);
                free_node(upper);
                return NULL;
            }
            if (upper_val != -1) {
                if (upper_val < 0 || upper_val < lower_val) {
                    free_node(lower);
                    free_node(upper);
                    return NULL;
                }
            }
            break;
        }
        default: {
            return NULL;
        }
    }

    Node *quantifier_node = create_node("<Quantifier>");
    add_child(quantifier_node, lower);
    add_child(quantifier_node, upper);
    return quantifier_node;
}

Node *atom(const char *pattern, size_t *pos) {
    const char *peek_ptr = peek(pattern, pos, 0);
    if (!peek_ptr || *peek_ptr == '\0') {
        return NULL;
    }

    Node *node = create_node("<Atom>");
    switch (*peek_ptr) {
        case '.': {
            match(pattern, pos, ".");
            add_child(node, create_node("<Dot>"));
            break;
        }
        case '\\': {
            match(pattern, pos, "\\");
            Node *child_node = atom_escape(pattern, pos);
            if (!child_node) {
                free_node(node);
                return NULL;
            }
            add_child(node, child_node);
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
        }
    }

    return node;
}

Node *atom_escape(const char *pattern, size_t *pos) {
    const char *peek_ptr = peek(pattern, pos, 0);
    if (!peek_ptr) {
        return NULL;
    }
    Node *node = NULL;
    const char peek_ch = *peek_ptr;

    switch (peek_ch) {
        case 'f':
        case 'n':
        case 'r':
        case 't':
        case 'v': {
            match(pattern, pos, peek_ptr);
            node = create_node("<Control>");
            add_child(node, create_node(peek_ptr));
            break;
        }

        case 'd':
        case 'D':
        case 's':
        case 'S':
        case 'w':
        case 'W': {
            match(pattern, pos, peek_ptr);
            node = create_node("<Perl>");
            add_child(node, create_node(peek_ptr));
            break;
        }

        case 'x': {
            match(pattern, pos, "x");
            Node *hex = hex_sequence(pattern, pos);
            if (!hex) {
                return NULL;
            }
            node = create_node("<HexSeq>");
            add_child(node, hex);
            break;
        }

        case 'p':
        case 'P': {
            Node *uni = unicode_sequence(pattern, pos);
            if (!uni) {
                return NULL;
            }
            node = create_node("<UniSeq>");
            add_child(node, uni);
            break;
        }

        default: {
            return NULL;
        }
    }

    return node;
}

Node *hex_sequence(const char *pattern, size_t *pos) {
    if (!match(pattern, pos, "{")) {
        return NULL;
    }
    const char *peek_ptr = peek(pattern, pos, 0);
    if (!peek_ptr || !is_hex(*peek_ptr)) {
        return NULL;
    }
    Node *node = create_node("");
    size_t len = 0;
    while (len < 4 && *peek_ptr && is_hex(*peek_ptr)) {
        node->label[len++] = *peek_ptr;
        peek_ptr = next(pattern, pos, 0);
    }

    if (!match(pattern, pos, "}")) {
        free_node(node);
        return NULL;
    }
    return node;
}

Node *unicode_sequence(const char *pattern, size_t *pos) {
    const char *p_char = peek(pattern, pos, 0);
    if (!p_char || (*p_char != 'p' && *p_char != 'P')) {
        return NULL;
    }
    const bool is_negated = *p_char == 'P';
    if (!match(pattern, pos, p_char)) {
        return NULL;
    }

    if (!match(pattern, pos, "{")) {
        return NULL;
    }

    const char *upper_ptr = peek(pattern, pos, 0);
    if (!upper_ptr || !is_upercase_letter(*upper_ptr)) {
        return NULL;
    }
    const char upper = *upper_ptr;
    if (!match(pattern, pos, upper_ptr)) {
        return NULL;
    }

    const char *lower_ptr = peek(pattern, pos, 0);
    if (!lower_ptr || !is_lowercase_letter(*lower_ptr)) {
        return NULL;
    }
    const char lower = *lower_ptr;
    if (!match(pattern, pos, lower_ptr)) {
        return NULL;
    }

    if (!match(pattern, pos, "}")) {
        return NULL;
    }

    Node *node = create_node(is_negated ? "^" : "");
    const size_t base = is_negated ? 1 : 0;
    node->label[base] = upper;
    node->label[base + 1] = lower;
    node->label[base + 2] = '\0';

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

Node *decimal_digits(const char *pattern, size_t *pos) {
    const char *peek_ptr = peek(pattern, pos, 0);
    if (!peek_ptr || !is_digit(*peek_ptr)) {
        return NULL;
    }
    Node *node = create_node("");

    for (size_t i = 0; i < 16; i++) {
        node->label[i] = *peek_ptr;
        *pos += 1;
        peek_ptr = peek(pattern, pos, 0);
        if (!peek_ptr) {
            return NULL;
        }
        if (!is_digit(*peek_ptr)) {
            break;
        }
    }

    return node;
}

Node *build_syntax_tree(const char *pattern) {
    size_t pos = 0;

    Node *res = root(pattern, &pos);
    return res;
}
