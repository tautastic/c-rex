#pragma once

#include "utf8.h"
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#define MAX_RUNE 0x10FFFFU

typedef struct {
    rune *data;
    size_t length;
    size_t capacity;
    bool need_free;
} RuneRange;

void rrange_init(RuneRange *rr);
void rrange_free(RuneRange *rr);

bool append_range(RuneRange *rr, rune lo, rune hi);
bool append_literal(RuneRange *rr, rune ch);
bool append_class(RuneRange *rr0, const RuneRange *rr1);
void negate_class(RuneRange *rr);
void clean_class(RuneRange *rr);

void rrange_print(const RuneRange *rr);
void rrange_print_short(const RuneRange *rr);
