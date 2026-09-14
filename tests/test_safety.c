#include "dacb.h"

#include <assert.h>
#include <stdint.h>
#include <stdio.h>

int main(void) {
    const uint32_t states[] = {0, 1, UINT32_MAX, UINT32_C(1015568748), UINT32_C(4125282121)};
    for (size_t i = 0; i < sizeof(states) / sizeof(states[0]); ++i) {
        const uint32_t y = dacb_step32(states[i]);
        assert(dacb_prev32(y) == states[i]);
    }

    dacb_summary truth;
    assert(dacb_run(DACB_SEED, 5000, &truth) == 0);
    assert(truth.last == UINT32_C(4125282121));
    assert(truth.sum_mod == UINT32_C(585303980));
    assert(truth.weighted_mod == UINT32_C(554861868));

    dacb_candidate bad = {UINT32_C(3845503477), UINT64_C(4566409260), UINT64_C(1247804322), true, true, 2};
    assert(dacb_audit_candidate(&truth, &bad) != 0);

    dacb_candidate corrected;
    uint32_t issues = 0;
    assert(dacb_self_correct(5000, &bad, &corrected, &issues) == 1);
    assert(corrected.last == truth.last);
    assert(corrected.sum_mod == truth.sum_mod);
    assert(corrected.weighted_mod == truth.weighted_mod);

    dacb_candidate stale = {truth.last, truth.sum_mod, truth.weighted_mod, false, true, 3};
    assert(dacb_self_correct(5000, &stale, &corrected, &issues) == 2);

    dacb_lattice_result preserve, plus;
    assert(dacb_lattice_run(DACB_TIE_PRESERVE, 200, &preserve) == 0);
    assert(dacb_lattice_run(DACB_TIE_PLUS, 200, &plus) == 0);
    assert(preserve.magnetization == -1816);
    assert(plus.magnetization == 4096);

    puts("DACB_SAFETY_TESTS_PASS");
    return 0;
}
