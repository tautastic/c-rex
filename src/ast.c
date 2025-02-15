#include "ast.h"

void print_tree(Node *node, const char *prefix, const int is_last) {
    if (!node) {
        return;
    }

    const char *branch = is_last ? "└── " : "├── ";
    printf("%s%s%s\n", prefix, branch, node->label);

    char new_prefix[256];
    snprintf(new_prefix, sizeof(new_prefix), "%s%s", prefix, is_last ? "    " : "│   ");

    for (size_t i = 0; i < node->sub_count; i++) {
        const int child_is_last = i == node->sub_count - 1;
        print_tree(node->sub[i], new_prefix, child_is_last);
    }
}

Node *create_node(const char *label) {
    Node *node = malloc(sizeof(Node));
    strncpy(node->label, label, 15);
    node->label[15] = '\0';
    node->sub = NULL;
    node->sub_count = 0;
    return node;
}

void free_node(Node *node) {
    if (!node) {
        return;
    }

    if (node->sub) {
        for (size_t i = 0; i < node->sub_count; i++) {
            free_node(node->sub[i]);
        }
        free(node->sub);
    }

    free(node);
}

void add_child(Node *parent, Node *child) {
    if (!parent || !child) {
        return;
    }

    Node **new_sub = realloc(parent->sub, sizeof(Node *) * (parent->sub_count + 1));
    if (!new_sub) {
        return;
    }
    parent->sub = new_sub;
    parent->sub[parent->sub_count] = child;
    parent->sub_count++;
}
