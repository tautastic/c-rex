#include "rrange.h"

static int compare_ranges(const void *a, const void *b) {
    const rune *range_a = a;
    const rune *range_b = b;
    if (range_a[0] < range_b[0]) return -1;
    if (range_a[0] > range_b[0]) return 1;
    if (range_a[1] > range_b[1]) return -1;
    if (range_a[1] < range_b[1]) return 1;
    return 0;
}

void rrange_init(RuneRange *rr) {
    rr->data = NULL;
    rr->length = 0;
    rr->capacity = 0;
    rr->need_free = true;
}

void rrange_free(RuneRange *rr) {
    if (rr->need_free) {
        free(rr->data);
    }
    rr->data = NULL;
    rr->length = 0;
    rr->capacity = 0;
}

static bool ensure_capacity(RuneRange *rr, const size_t additional) {
    const size_t required = rr->length + additional;
    if (required <= rr->capacity) {
        return true;
    }

    size_t new_cap = rr->capacity == 0 ? 4 : rr->capacity;
    while (new_cap < required) {
        new_cap *= 2;
    }

    rune *new_data = realloc(rr->data, new_cap * sizeof(rune));
    if (!new_data) {
        fprintf(stderr, "Memory allocation failed\n");
        return false;
    }

    rr->data = new_data;
    rr->capacity = new_cap;
    return true;
}

bool append_range(RuneRange *rr, const rune lo, const rune hi) {
    if (hi < lo) {
        fprintf(stderr, "Invalid class range: hi < lo\n");
        return false;
    }

    const size_t len = rr->length;
    for (size_t i = 2; i <= 4; i += 2) {
        if (len >= i) {
            const size_t idx = len - i;
            const rune rlo = rr->data[idx];
            const rune rhi = rr->data[idx + 1];
            if (lo <= rhi + 1 && rlo <= hi + 1) {
                rr->data[idx] = lo < rlo ? lo : rlo;
                rr->data[idx + 1] = hi > rhi ? hi : rhi;
                return true;
            }
        }
    }

    if (!ensure_capacity(rr, 2)) {
        return false;
    }

    rr->data[rr->length] = lo;
    rr->data[rr->length + 1] = hi;
    rr->length += 2;
    return true;
}

bool append_literal(RuneRange *rr, const rune ch) {
    return append_range(rr, ch, ch);
}

bool append_class(RuneRange *rr0, const RuneRange *rr1) {
    for (size_t i = 0; i < rr1->length; i += 2) {
        if (!append_range(rr0, rr1->data[i], rr1->data[i + 1])) {
            return false;
        }
    }
    return true;
}

void negate_class(RuneRange *rr) {
    rune next_lo = 0;
    size_t w = 0;

    for (size_t i = 0; i < rr->length; i += 2) {
        const rune lo = rr->data[i];
        const rune hi = rr->data[i + 1];
        if (next_lo <= lo - 1) {
            if (!ensure_capacity(rr, 2)) {
                return;
            }
            rr->data[w] = next_lo;
            rr->data[w + 1] = lo - 1;
            w += 2;
        }
        next_lo = hi + 1;
    }

    if (next_lo <= MAX_RUNE) {
        if (!ensure_capacity(rr, 2)) {
            return;
        }
        rr->data[w] = next_lo;
        rr->data[w + 1] = MAX_RUNE;
        w += 2;
    }

    rr->length = w;
}

void clean_class(RuneRange *rr) {
    const size_t num_ranges = rr->length / 2;
    if (num_ranges == 0) {
        return;
    }

    qsort(rr->data, num_ranges, 2 * sizeof(rune), compare_ranges);

    size_t w = 2;
    for (size_t i = 2; i < rr->length; i += 2) {
        const rune lo = rr->data[i];
        const rune hi = rr->data[i + 1];
        const rune prev_hi = rr->data[w - 1];

        if (lo <= prev_hi + 1) {
            if (hi > prev_hi) {
                rr->data[w - 1] = hi;
            }
        } else {
            rr->data[w] = lo;
            rr->data[w + 1] = hi;
            w += 2;
        }
    }

    rr->length = w;
}

void rrange_print(const RuneRange *rr) {
    for (size_t i = 0; i < rr->length; i += 2) {
        const rune lo = rr->data[i];
        const rune hi = rr->data[i + 1];

        printf("{0x%x, 0x%x}: ", lo, hi);

        for (rune cp = lo; cp <= hi; cp++) {
            utf8_int8_t buffer[5] = {0};
            utf8catcodepoint(buffer, cp, sizeof(buffer) - 1);

            printf("%s", buffer);

            if (cp != hi) {
                printf(" ");
            }
        }
        printf("\n");
    }
}

void rrange_print_short(const RuneRange *rr) {
    for (size_t i = 0; i < rr->length; i += 2) {
        printf("\t0x%x, 0x%x,\n", rr->data[i], rr->data[i + 1]);
    }
}

