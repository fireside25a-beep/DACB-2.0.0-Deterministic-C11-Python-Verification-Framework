#ifndef DACB_H
#define DACB_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

#define DACB_VERSION "2.0.0"
#define DACB_A UINT32_C(1664525)
#define DACB_C UINT32_C(1013904223)
#define DACB_SEED UINT32_C(1)
#define DACB_CHECK_MOD UINT32_C(1000000007)

typedef struct {
    uint32_t mul;
    uint32_t add;
} dacb_affine32;

typedef struct {
    uint32_t a00, a01, a10, a11;
} dacb_matrix2;

typedef struct {
    uint64_t steps;
    uint32_t seed;
    uint32_t last;
    uint32_t affine_last;
    uint32_t matrix_last;
    uint32_t reverse_seed;
    uint32_t sum_mod;
    uint32_t weighted_mod;
} dacb_summary;

typedef struct {
    uint64_t cases;
    uint64_t fnv64;
    uint32_t modsum;
} dacb_exhaustive_result;

typedef enum {
    DACB_TIE_PRESERVE = 0,
    DACB_TIE_PLUS = 1
} dacb_tie_policy;

typedef struct {
    int32_t magnetization;
    int32_t energy;
    uint32_t weighted_checksum;
    uint32_t rolling_checksum;
    uint32_t convergence_step;
    uint32_t first_flips;
    uint32_t second_flips;
    uint32_t third_flips;
} dacb_lattice_result;

enum {
    DACB_AUDIT_LAST_MISMATCH       = 1u << 0,
    DACB_AUDIT_SUM_OUT_OF_RANGE    = 1u << 1,
    DACB_AUDIT_SUM_MISMATCH        = 1u << 2,
    DACB_AUDIT_WEIGHT_OUT_OF_RANGE = 1u << 3,
    DACB_AUDIT_WEIGHT_MISMATCH     = 1u << 4,
    DACB_AUDIT_SOURCE_MISMATCH     = 1u << 5,
    DACB_AUDIT_INSUFFICIENT_ORACLE = 1u << 6,
    DACB_AUDIT_PROGRAM_MISMATCH    = 1u << 7
};

typedef struct {
    uint32_t last;
    uint64_t sum_mod;
    uint64_t weighted_mod;
    bool source_matches;
    bool program_matches;
    unsigned independent_oracles;
} dacb_candidate;

uint32_t dacb_step32(uint32_t x);
uint32_t dacb_inverse_multiplier32(void);
uint32_t dacb_prev32(uint32_t x);

dacb_affine32 dacb_affine_compose(dacb_affine32 f, dacb_affine32 g);
dacb_affine32 dacb_affine_pow(uint64_t n);
uint32_t dacb_affine_state(uint32_t seed, uint64_t n);

dacb_matrix2 dacb_matrix_mul(dacb_matrix2 a, dacb_matrix2 b);
dacb_matrix2 dacb_matrix_pow(uint64_t n);
uint32_t dacb_matrix_state(uint32_t seed, uint64_t n);

int dacb_run(uint32_t seed, uint64_t steps, dacb_summary *out);
int dacb_exhaustive(unsigned min_bits, unsigned max_bits, dacb_exhaustive_result *out);
int dacb_lattice_run(dacb_tie_policy policy, unsigned max_steps, dacb_lattice_result *out);
uint32_t dacb_audit_candidate(const dacb_summary *truth, const dacb_candidate *candidate);
int dacb_self_correct(uint64_t steps, const dacb_candidate *candidate, dacb_candidate *corrected, uint32_t *issues);
int dacb_write_canonical_report(FILE *stream);

#endif
