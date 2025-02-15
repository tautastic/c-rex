#pragma once

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

typedef struct Node {
    char label[16];
    struct Node **sub;
    size_t sub_count;
} Node;

void print_tree(Node *node, const char *prefix, const int is_last);

Node *create_node();

void add_child(Node *parent, Node *child);

void free_node(Node *node);

