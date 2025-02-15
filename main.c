#include "lexer.h"

int main(void) {
    const char *pattern = "(a{2,4}b?)?";

    Node *root = build_syntax_tree(pattern);

    print_tree(root, "", 1);
    free_node(root);

    return 0;
}
