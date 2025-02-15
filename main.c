#include "rrange.h"
#include "charclass.h"

int main(void) {
    RuneRange rr = PERL_DIGIT;

    rrange_print(&rr);
    rrange_free(&rr);

    return 0;
}
