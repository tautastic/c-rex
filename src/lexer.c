#include "lexer.h"

bool is_special(const char c) {
    return c == '.' || c == '^' || c == '$' || c == '*' || c == '+' || c == '?' ||
           c == '(' || c == ')' || c == '[' || c == '{' || c == '\\' || c == '|' || c == ']';
}

bool is_special_in_class(const char c) {
    return c == '^' || c == '-' || c == ']' || c == '\\';
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
    return ('0' <= c && c <= '9') ||
           ('a' <= c && c <= 'f') ||
           ('A' <= c && c <= 'F');
}

bool is_end_delimiter(const char c) {
    return c == '\0' || c == '|' || c == ')' || c == '}' || c == ']';
}

const char *peek(const char *pattern, const size_t *pos, const size_t lookahead) {
    if (!pattern || !pos) {
        PARSE_ERROR("NULL pointer argument");
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
        PARSE_ERROR("NULL pointer argument");
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
        PARSE_ERROR("NULL pointer argument");
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
        PARSE_ERROR("NULL pointer argument");
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

    const char *peek_ptr = peek(pattern, pos, 0);
    while (peek_ptr && *peek_ptr != '\0') {
        if (match(pattern, pos, ")")) {
            break;
        }

        if (match(pattern, pos, "|")) {
            add_child(node, branch(pattern, pos));
        } else {
            add_child(branch_node, piece(pattern, pos));
        }
        peek_ptr = peek(pattern, pos, 0);
    }

    return node;
}

Node *branch(const char *pattern, size_t *pos) {
    Node *node = create_node("<Branch>");

    const char *peek_ptr = peek(pattern, pos, 0);
    while (peek_ptr) {
        if (is_end_delimiter(*peek_ptr)) {
            break;
        }
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
                    PARSE_ERROR("Syntax error: Missing lower and upper bound in quantifier");
                }
                lower = lower_part ? lower_part : create_node("0");
                upper = upper_part ? upper_part : create_node("-1");
            } else {
                if (!lower_part) {
                    PARSE_ERROR("Syntax error: Missing lower bound in quantifier");
                }
                upper = create_node(lower_part->label);
                lower = lower_part;
            }
            if (!match(pattern, pos, "}")) {
                free_node(lower);
                free_node(upper);
                PARSE_ERROR("Syntax error: Missing '}' in quantifier");
            }
            const int lower_val = atoi(lower->label);
            const int upper_val = atoi(upper->label);
            if (lower_val < 0) {
                free_node(lower);
                free_node(upper);
                PARSE_ERROR("Syntax error: Lower bound must be a non-negative integer");
            }
            if (upper_val != -1) {
                if (upper_val < 0 || upper_val < lower_val) {
                    free_node(lower);
                    free_node(upper);
                    PARSE_ERROR("Syntax error: Upper bound must be a non-negative integer greater than or equal to lower bound");
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

    Node *node = NULL;
    switch (*peek_ptr) {
        case '.': {
            match(pattern, pos, ".");
            node = create_node("<Atom>");
            add_child(node, create_node("<Dot>"));
            break;
        }
        case '\\': {
            match(pattern, pos, "\\");
            Node *child_node = atom_escape(pattern, pos);
            if (!child_node) {
                return NULL;
            }
            node = create_node("<Atom>");
            add_child(node, child_node);
            break;
        }
        case '[': {
            Node *class_node = char_class(pattern, pos);
            if (!class_node) {
                return NULL;
            }
            node = create_node("<Atom>");
            add_child(node, class_node);
            break;
        }
        case '(': {
            match(pattern, pos, "(");
            Node *expr_node = expr(pattern, pos);
            match(pattern, pos, ")");
            node = create_node("<Atom>");
            add_child(node, expr_node);
            break;
        }
        default: {
            if (is_special(*peek_ptr)) {
                return NULL;
            }
            node = create_node("<Atom>");
            add_child(node, literal(pattern, pos));

            return node;
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
                PARSE_ERROR("Syntax error: Invalid hex sequence");
            }
            node = create_node("<HexSeq>");
            add_child(node, hex);
            break;
        }

        case 'p':
        case 'P': {
            Node *uni = unicode_sequence(pattern, pos);
            if (!uni) {
                PARSE_ERROR("Syntax error: Invalid unicode sequence");
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
        PARSE_ERROR("Syntax error: Missing '{' in hex sequence");
    }
    const char *peek_ptr = peek(pattern, pos, 0);
    if (!peek_ptr || !is_hex(*peek_ptr)) {
        PARSE_ERROR("Syntax error: Invalid hex sequence");
    }
    Node *node = create_node("");
    size_t len = 0;
    while (len < 4 && *peek_ptr && is_hex(*peek_ptr)) {
        node->label[len++] = *peek_ptr;
        peek_ptr = next(pattern, pos, 0);
    }

    if (!match(pattern, pos, "}")) {
        free_node(node);
        PARSE_ERROR("Syntax error: Missing '}' in hex sequence");
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

Node *char_class(const char *pattern, size_t *pos) {
    if (!match(pattern, pos, "[")) {
        return NULL;
    }

    Node *node = create_node("");
    const char *peek_ptr = peek(pattern, pos, 0);
    while (peek_ptr && *peek_ptr != '\0' && *peek_ptr != ']') {
        if (match(pattern, pos, "^")) {
            node->label[0] = '^';
        }
        Node *clr = class_range(pattern, pos);
        if (!clr) {
            free_node(node);
            // We try to consume the closing bracket just in case
            // that parsing can go on (tbh we might as well abort).
            match(pattern, pos, "]");
            return NULL;
        }
        if (clr->sub_count == 1) {
            clr = clr->sub[0];
        }
        add_child(node, clr);
        peek_ptr = peek(pattern, pos, 0);
    }

    if (!match(pattern, pos, "]")) {
        free_node(node);
        return NULL;
    }

    return node;

}

Node *class_range(const char *pattern, size_t *pos) {
    Node *node = create_node("ClassRange");
    Node *cla0 = class_atom(pattern, pos);
    if (!cla0) {
        free_node(node);
        return NULL;
    }
    add_child(node, cla0);
    if (match(pattern, pos, "-")) {
        Node *cla1 = class_atom(pattern, pos);
        if (!cla1) {
            free_node(node);
            return NULL;
        }
        if (utf8casecmp(cla0->label, "<Perl>") == 0 || utf8casecmp(cla0->label, "<UniSeq>") == 0 ||
            utf8casecmp(cla1->label, "<Perl>") == 0 || utf8casecmp(cla1->label, "<UniSeq>") == 0) {
                free_node(cla1);
                free_node(node);
                return NULL;
            }
        add_child(node, cla1);
    }

    return node;
}

Node *class_atom(const char *pattern, size_t *pos) {
    if (match(pattern, pos, "\\")) {
        return atom_escape(pattern, pos);
    }

    const char *peek_ptr = peek(pattern, pos, 0);
    if (!peek_ptr || *peek_ptr == '\0' || *peek_ptr == '\\' || *peek_ptr == ']' || *peek_ptr == '-') {
        return NULL;
    }

    return literal(pattern, pos);
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
    if (!res) {
        PARSE_ERROR("Syntax error: Invalid pattern");
    }
    return res;
}
