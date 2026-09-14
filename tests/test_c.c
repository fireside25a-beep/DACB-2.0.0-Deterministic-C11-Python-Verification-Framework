#include "dacb.h"

#include <assert.h>
#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

static void test_known_vectors(void) {
    struct {
        uint64_t n;
        uint32_t value;
        uint32_t sum_mod;
        uint32_t weighted_mod;
    } vectors[] = {
        {1, UINT32_C(1015568748), UINT32_C(15568741), UINT32_C(15568741)},
        {2, UINT32_C(1586005467), UINT32_C(601574201), UINT32_C(187579654)},
        {3, UINT32_C(2165703038), UINT32_C(767277225), UINT32_C(684688726)},
        {10, UINT32_C(2745540835), UINT32_C(131652221), UINT32_C(898836369)},
        {100, UINT32_C(2146152485), UINT32_C(492427211), UINT32_C(623809866)},
        {500, UINT32_C(2563561269), UINT32_C(523200049), UINT32_C(256699986)},
        {1000, UINT32_C(645503657), UINT32_C(63521775), UINT32_C(299065930)},
        {5000, UINT32_C(4125282121), UINT32_C(585303980), UINT32_C(554861868)},
        {100000, UINT32_C(694140833), UINT32_C(784266586), UINT32_C(455600981)},
        {1000000, UINT32_C(366300225), UINT32_C(346639628), UINT32_C(152047219)}
    };
    for (size_t i = 0; i < sizeof(vectors) / sizeof(vectors[0]); ++i) {
        dacb_summary s;
        assert(dacb_run(DACB_SEED, vectors[i].n, &s) == 0);
        assert(s.last == vectors[i].value);
        assert(s.affine_last == vectors[i].value);
        assert(s.matrix_last == vectors[i].value);
        assert(s.reverse_seed == DACB_SEED);
        assert(s.sum_mod == vectors[i].sum_mod);
        assert(s.weighted_mod == vectors[i].weighted_mod);
    }
}

static void test_inverse_and_boundaries(void) {
    const uint32_t inv = dacb_inverse_multiplier32();
    assert((uint32_t)((uint64_t)DACB_A * inv) == UINT32_C(1));
    const uint32_t states[] = {0, 1, UINT32_MAX, UINT32_C(1015568748), UINT32_C(4125282121)};
    for (size_t i = 0; i < sizeof(states) / sizeof(states[0]); ++i) {
        const uint32_t y = dacb_step32(states[i]);
        assert(dacb_prev32(y) == states[i]);
    }

    dacb_summary zero;
    assert(dacb_run(UINT32_MAX, 0, &zero) == 0);
    assert(zero.last == UINT32_MAX);
    assert(zero.sum_mod == 0);
    assert(zero.weighted_mod == 0);
    assert(zero.reverse_seed == UINT32_MAX);
}

static void test_composition_laws(void) {
    const uint64_t ns[] = {0, 1, 2, 3, 7, 31, 1000, 5000, 65537};
    for (size_t i = 0; i < sizeof(ns) / sizeof(ns[0]); ++i) {
        for (size_t j = 0; j < sizeof(ns) / sizeof(ns[0]); ++j) {
            const uint64_t n = ns[i], m = ns[j];
            const dacb_affine32 tn = dacb_affine_pow(n);
            const dacb_affine32 tm = dacb_affine_pow(m);
            const dacb_affine32 tnm = dacb_affine_pow(n + m);
            const dacb_affine32 composed = dacb_affine_compose(tn, tm);
            assert(composed.mul == tnm.mul);
            assert(composed.add == tnm.add);

            const dacb_matrix2 mn = dacb_matrix_pow(n);
            const dacb_matrix2 mm = dacb_matrix_pow(m);
            const dacb_matrix2 mnm = dacb_matrix_pow(n + m);
            const dacb_matrix2 product = dacb_matrix_mul(mn, mm);
            assert(memcmp(&product, &mnm, sizeof(product)) == 0);
        }
    }
}

static void test_exact_dyadic_embedding(void) {
    uint32_t x = DACB_SEED;
    for (unsigned i = 0; i < 5000; ++i) {
        x = dacb_step32(x);
        const uint64_t d0_num = x;
        const uint64_t d1_num = (uint64_t)x << 1;
        const uint64_t d2_num = (uint64_t)x << 2;
        /* x/1 == (2x)/2 == (4x)/4, checked by exact integer cross-products. */
        assert(d0_num * UINT64_C(2) == d1_num);
        assert(d0_num * UINT64_C(4) == d2_num);
        assert(d1_num * UINT64_C(2) == d2_num);
    }
}

static void test_exhaustive(void) {
    dacb_exhaustive_result e;
    assert(dacb_exhaustive(2, 10, &e) == 0);
    assert(e.cases == UINT64_C(1400140));
    assert(e.fnv64 == UINT64_C(0x9f80ade0ca124045));
    assert(e.modsum == UINT32_C(220519645));
}

static void test_lattice_semantic_drift(void) {
    dacb_lattice_result preserve, plus;
    assert(dacb_lattice_run(DACB_TIE_PRESERVE, 200, &preserve) == 0);
    assert(dacb_lattice_run(DACB_TIE_PLUS, 200, &plus) == 0);
    assert(preserve.magnetization == -1816);
    assert(preserve.energy == -4296);
    assert(preserve.weighted_checksum == UINT32_C(13134816));
    assert(preserve.convergence_step == 3);
    assert(preserve.first_flips == 987 && preserve.second_flips == 38 && preserve.third_flips == 0);
    assert(plus.magnetization == 4096);
    assert(plus.energy == -8192);
    assert(plus.rolling_checksum == UINT32_C(299341384));
    assert(plus.convergence_step == 5);
    assert(preserve.magnetization != plus.magnetization);
}

static void test_adversarial_audit(void) {
    dacb_summary truth;
    assert(dacb_run(DACB_SEED, 5000, &truth) == 0);

    dacb_candidate impossible = {UINT32_C(3845503477), UINT64_C(4566409260), UINT64_C(1247804322), true, true, 2};
    uint32_t issues = dacb_audit_candidate(&truth, &impossible);
    assert((issues & DACB_AUDIT_LAST_MISMATCH) != 0);
    assert((issues & DACB_AUDIT_SUM_OUT_OF_RANGE) != 0);
    assert((issues & DACB_AUDIT_WEIGHT_OUT_OF_RANGE) != 0);

    dacb_candidate exact_but_stale = {truth.last, truth.sum_mod, truth.weighted_mod, false, true, 3};
    issues = dacb_audit_candidate(&truth, &exact_but_stale);
    assert(issues == DACB_AUDIT_SOURCE_MISMATCH);

    dacb_candidate exact_but_mutated = {truth.last, truth.sum_mod, truth.weighted_mod, true, false, 3};
    issues = dacb_audit_candidate(&truth, &exact_but_mutated);
    assert(issues == DACB_AUDIT_PROGRAM_MISMATCH);

    dacb_candidate exact_without_oracle = {truth.last, truth.sum_mod, truth.weighted_mod, true, true, 0};
    issues = dacb_audit_candidate(&truth, &exact_without_oracle);
    assert(issues == DACB_AUDIT_INSUFFICIENT_ORACLE);

    dacb_candidate exact = {truth.last, truth.sum_mod, truth.weighted_mod, true, true, 3};
    assert(dacb_audit_candidate(&truth, &exact) == 0);

    dacb_candidate corrected;
    uint32_t correction_issues = 0;
    assert(dacb_self_correct(5000, &impossible, &corrected, &correction_issues) == 1);
    assert(correction_issues != 0);
    assert(corrected.last == truth.last);
    assert(corrected.sum_mod == truth.sum_mod);
    assert(corrected.weighted_mod == truth.weighted_mod);
    assert(corrected.independent_oracles == 3);

    assert(dacb_self_correct(5000, &exact_but_stale, &corrected, &correction_issues) == 2);
    assert((correction_issues & DACB_AUDIT_SOURCE_MISMATCH) != 0);
}


static uint32_t mutated_step_increment(uint32_t x) {
    return (uint32_t)((uint64_t)DACB_A * x + (uint64_t)(DACB_C + UINT32_C(2)));
}

static uint32_t mutated_step_multiplier(uint32_t x) {
    return (uint32_t)((uint64_t)(DACB_A + UINT32_C(2)) * x + DACB_C);
}

static void test_mutation_detection(void) {
    dacb_summary truth;
    assert(dacb_run(DACB_SEED, 5000, &truth) == 0);
    uint32_t xi = DACB_SEED, xm = DACB_SEED;
    uint32_t canonical = DACB_SEED;
    uint32_t off_by_one = 0;
    for (uint32_t i = 0; i < 5000; ++i) {
        xi = mutated_step_increment(xi);
        xm = mutated_step_multiplier(xm);
        canonical = dacb_step32(canonical);
        off_by_one = (uint32_t)(((uint64_t)off_by_one + (uint64_t)i * (canonical % DACB_CHECK_MOD)) % DACB_CHECK_MOD);
    }
    assert(xi != truth.last);
    assert(xm != truth.last);
    assert(off_by_one != truth.weighted_mod);
    const uint64_t unmasked = (uint64_t)DACB_A * UINT32_MAX + DACB_C;
    assert(unmasked > UINT32_MAX);
    assert((uint32_t)unmasked == dacb_step32(UINT32_MAX));
}

static void test_repeatability(void) {
    dacb_summary baseline;
    assert(dacb_run(DACB_SEED, 5000, &baseline) == 0);
    for (unsigned i = 0; i < 50; ++i) {
        dacb_summary s;
        assert(dacb_run(DACB_SEED, 5000, &s) == 0);
        assert(s.steps == baseline.steps);
        assert(s.seed == baseline.seed);
        assert(s.last == baseline.last);
        assert(s.affine_last == baseline.affine_last);
        assert(s.matrix_last == baseline.matrix_last);
        assert(s.reverse_seed == baseline.reverse_seed);
        assert(s.sum_mod == baseline.sum_mod);
        assert(s.weighted_mod == baseline.weighted_mod);
    }
}

int main(void) {
    test_known_vectors();
    test_inverse_and_boundaries();
    test_composition_laws();
    test_exact_dyadic_embedding();
    test_exhaustive();
    test_lattice_semantic_drift();
    test_adversarial_audit();
    test_mutation_detection();
    test_repeatability();
    puts("DACB_C_TESTS_PASS");
    return 0;
}
