#include <stdio.h>
#include <assert.h>
#include "lexer.h"

int is_valid_regex(const char *pattern) {
    Node *result = build_syntax_tree(pattern);

    if (!result) {
        return 0;
    }

    free_node(result);
    return 1;
}

int main() {
    const char *valid_patterns[] = {
            "a*|b+|c?",
            "(ab|cd)*",
            "[^a-zA-Z0-9]",
            "[a-dm-p]",
            "a{2,4}",
            "\\d{3}-\\d+",
            "[a-z]{3,}",
            "(a|b){2,}",
            ".*abc",
            "\\w+\\S",
            "\\x{0041}",
            "[a-z]{,5}",
    };

    const char *invalid_patterns[] = {
            "[a-z-[ae]]",
            "(?:abc)?",
            "(?<=abc)d",
            "(?<!abc)d",
    };

    for (size_t i = 0; i < sizeof(valid_patterns) / sizeof(valid_patterns[0]); i++) {
        const char *pattern = valid_patterns[i];
        assert(is_valid_regex(pattern) == 1);
        printf("Pattern '%s' is valid.\n", pattern);
    }

    for (size_t i = 0; i < sizeof(invalid_patterns) / sizeof(invalid_patterns[0]); i++) {
        const char *pattern = invalid_patterns[i];
        assert(is_valid_regex(pattern) == 0);
        printf("Pattern '%s' is invalid as expected.\n", pattern);
    }

    printf("All regex pattern tests passed!\n");
    return 0;
}

