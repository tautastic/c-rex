#include "lexer.h"

int main(void) {
    const char *pattern = "a(b|c)";

    Node *root = build_syntax_tree(pattern);

    print_tree(root, "", 1);
    free_node(root);

    return 0;
}
