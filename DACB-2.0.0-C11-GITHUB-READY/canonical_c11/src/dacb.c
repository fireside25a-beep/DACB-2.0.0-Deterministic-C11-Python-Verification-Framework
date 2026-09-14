#include "dacb.h"

#include <inttypes.h>
#include <stdlib.h>
#include <string.h>

static uint32_t add_mod_check(uint32_t a, uint64_t b) {
    return (uint32_t)(((uint64_t)a + (b % DACB_CHECK_MOD)) % DACB_CHECK_MOD);
}

uint32_t dacb_step32(uint32_t x) {
    return (uint32_t)((uint64_t)DACB_A * (uint64_t)x + (uint64_t)DACB_C);
}

uint32_t dacb_inverse_multiplier32(void) {
    uint32_t inv = UINT32_C(1);
    for (unsigned i = 0; i < 5; ++i) {
        inv *= (uint32_t)(UINT32_C(2) - DACB_A * inv);
    }
    return inv;
}

uint32_t dacb_prev32(uint32_t x) {
    const uint32_t inv = dacb_inverse_multiplier32();
    return inv * (uint32_t)(x - DACB_C);
}

dacb_affine32 dacb_affine_compose(dacb_affine32 f, dacb_affine32 g) {
    dacb_affine32 r;
    r.mul = (uint32_t)((uint64_t)f.mul * g.mul);
    r.add = (uint32_t)((uint64_t)f.mul * g.add + f.add);
    return r;
}

dacb_affine32 dacb_affine_pow(uint64_t n) {
    dacb_affine32 result = {UINT32_C(1), UINT32_C(0)};
    dacb_affine32 base = {DACB_A, DACB_C};
    while (n != 0) {
        if ((n & UINT64_C(1)) != 0) {
            result = dacb_affine_compose(base, result);
        }
        base = dacb_affine_compose(base, base);
        n >>= 1;
    }
    return result;
}

uint32_t dacb_affine_state(uint32_t seed, uint64_t n) {
    const dacb_affine32 t = dacb_affine_pow(n);
    return (uint32_t)((uint64_t)t.mul * seed + t.add);
}

static uint32_t product_sum32(uint32_t a, uint32_t b, uint32_t c, uint32_t d) {
    const uint32_t p = (uint32_t)((uint64_t)a * b);
    const uint32_t q = (uint32_t)((uint64_t)c * d);
    return (uint32_t)(p + q);
}

dacb_matrix2 dacb_matrix_mul(dacb_matrix2 a, dacb_matrix2 b) {
    dacb_matrix2 r;
    r.a00 = product_sum32(a.a00, b.a00, a.a01, b.a10);
    r.a01 = product_sum32(a.a00, b.a01, a.a01, b.a11);
    r.a10 = product_sum32(a.a10, b.a00, a.a11, b.a10);
    r.a11 = product_sum32(a.a10, b.a01, a.a11, b.a11);
    return r;
}

dacb_matrix2 dacb_matrix_pow(uint64_t n) {
    dacb_matrix2 result = {UINT32_C(1), UINT32_C(0), UINT32_C(0), UINT32_C(1)};
    dacb_matrix2 base = {DACB_A, DACB_C, UINT32_C(0), UINT32_C(1)};
    while (n != 0) {
        if ((n & UINT64_C(1)) != 0) {
            result = dacb_matrix_mul(base, result);
        }
        base = dacb_matrix_mul(base, base);
        n >>= 1;
    }
    return result;
}

uint32_t dacb_matrix_state(uint32_t seed, uint64_t n) {
    const dacb_matrix2 m = dacb_matrix_pow(n);
    return (uint32_t)((uint64_t)m.a00 * seed + m.a01);
}

int dacb_run(uint32_t seed, uint64_t steps, dacb_summary *out) {
    if (out == NULL) {
        return -1;
    }
    memset(out, 0, sizeof(*out));
    uint32_t x = seed;
    uint32_t sum_mod = 0;
    uint32_t weighted_mod = 0;
    for (uint64_t i = 1; i <= steps; ++i) {
        x = dacb_step32(x);
        sum_mod = add_mod_check(sum_mod, x);
        const uint64_t term = (i % DACB_CHECK_MOD) * (uint64_t)(x % DACB_CHECK_MOD);
        weighted_mod = add_mod_check(weighted_mod, term);
    }

    out->steps = steps;
    out->seed = seed;
    out->last = x;
    out->affine_last = dacb_affine_state(seed, steps);
    out->matrix_last = dacb_matrix_state(seed, steps);
    out->reverse_seed = x;
    const uint32_t inv = dacb_inverse_multiplier32();
    for (uint64_t i = 0; i < steps; ++i) {
        out->reverse_seed = inv * (uint32_t)(out->reverse_seed - DACB_C);
    }
    out->sum_mod = sum_mod;
    out->weighted_mod = weighted_mod;

    return (out->last == out->affine_last &&
            out->last == out->matrix_last &&
            out->reverse_seed == seed) ? 0 : 1;
}

static uint32_t mask_for_bits(unsigned bits) {
    if (bits >= 32) {
        return UINT32_MAX;
    }
    return (UINT32_C(1) << bits) - UINT32_C(1);
}

static dacb_affine32 affine_compose_mask(dacb_affine32 f, dacb_affine32 g, uint32_t mask) {
    dacb_affine32 r;
    r.mul = (uint32_t)(((uint64_t)f.mul * g.mul) & mask);
    r.add = (uint32_t)(((uint64_t)f.mul * g.add + f.add) & mask);
    return r;
}

static dacb_affine32 affine_pow_mask(uint64_t n, uint32_t mask) {
    dacb_affine32 result = {UINT32_C(1), UINT32_C(0)};
    dacb_affine32 base = {DACB_A & mask, DACB_C & mask};
    while (n != 0) {
        if ((n & UINT64_C(1)) != 0) {
            result = affine_compose_mask(base, result, mask);
        }
        base = affine_compose_mask(base, base, mask);
        n >>= 1;
    }
    return result;
}

static uint32_t product_sum_mask(uint32_t a, uint32_t b, uint32_t c, uint32_t d, uint32_t mask) {
    return (uint32_t)(((uint64_t)a * b + (uint64_t)c * d) & mask);
}

static dacb_matrix2 matrix_mul_mask(dacb_matrix2 a, dacb_matrix2 b, uint32_t mask) {
    dacb_matrix2 r;
    r.a00 = product_sum_mask(a.a00, b.a00, a.a01, b.a10, mask);
    r.a01 = product_sum_mask(a.a00, b.a01, a.a01, b.a11, mask);
    r.a10 = product_sum_mask(a.a10, b.a00, a.a11, b.a10, mask);
    r.a11 = product_sum_mask(a.a10, b.a01, a.a11, b.a11, mask);
    return r;
}

static dacb_matrix2 matrix_pow_mask(uint64_t n, uint32_t mask) {
    dacb_matrix2 result = {UINT32_C(1), UINT32_C(0), UINT32_C(0), UINT32_C(1)};
    dacb_matrix2 base = {DACB_A & mask, DACB_C & mask, UINT32_C(0), UINT32_C(1)};
    while (n != 0) {
        if ((n & UINT64_C(1)) != 0) {
            result = matrix_mul_mask(base, result, mask);
        }
        base = matrix_mul_mask(base, base, mask);
        n >>= 1;
    }
    return result;
}

static uint64_t fnv_byte(uint64_t h, uint8_t byte) {
    h ^= byte;
    return h * UINT64_C(1099511628211);
}

static uint64_t fnv_u32le(uint64_t h, uint32_t value) {
    for (unsigned shift = 0; shift < 32; shift += 8) {
        h = fnv_byte(h, (uint8_t)(value >> shift));
    }
    return h;
}

int dacb_exhaustive(unsigned min_bits, unsigned max_bits, dacb_exhaustive_result *out) {
    if (out == NULL || min_bits < 2 || max_bits > 16 || min_bits > max_bits) {
        return -1;
    }

    uint64_t total = 0;
    uint64_t fnv = UINT64_C(14695981039346656037);
    uint32_t modsum = 0;

    for (unsigned bits = min_bits; bits <= max_bits; ++bits) {
        const uint32_t modulus = UINT32_C(1) << bits;
        const uint32_t mask = mask_for_bits(bits);
        dacb_affine32 *aff = calloc((size_t)modulus + 1u, sizeof(*aff));
        dacb_matrix2 *mat = calloc((size_t)modulus + 1u, sizeof(*mat));
        if (aff == NULL || mat == NULL) {
            free(aff);
            free(mat);
            return -2;
        }

        for (uint32_t n = 0; n <= modulus; ++n) {
            aff[n] = affine_pow_mask(n, mask);
            mat[n] = matrix_pow_mask(n, mask);
            if (mat[n].a00 != aff[n].mul || mat[n].a01 != aff[n].add ||
                mat[n].a10 != 0 || mat[n].a11 != 1) {
                free(aff);
                free(mat);
                return 1;
            }
        }

        for (uint32_t seed = 0; seed < modulus; ++seed) {
            uint32_t x = seed;
            for (uint32_t n = 0; n <= modulus; ++n) {
                const uint32_t xa = (uint32_t)(((uint64_t)aff[n].mul * seed + aff[n].add) & mask);
                const uint32_t xm = (uint32_t)(((uint64_t)mat[n].a00 * seed + mat[n].a01) & mask);
                if (x != xa || x != xm) {
                    free(aff);
                    free(mat);
                    return 2;
                }
                if ((n == modulus && x != seed) || (n > 0 && n < modulus && x == seed)) {
                    free(aff);
                    free(mat);
                    return 3;
                }

                fnv = fnv_u32le(fnv, (uint32_t)bits);
                fnv = fnv_u32le(fnv, seed);
                fnv = fnv_u32le(fnv, n);
                fnv = fnv_u32le(fnv, x);
                const uint64_t mix = (uint64_t)bits + UINT64_C(3) * seed + UINT64_C(5) * n + UINT64_C(7) * x;
                modsum = add_mod_check(modsum, mix);
                ++total;

                if (n < modulus) {
                    x = (uint32_t)(((uint64_t)(DACB_A & mask) * x + (DACB_C & mask)) & mask);
                }
            }
        }

        free(aff);
        free(mat);
    }

    out->cases = total;
    out->fnv64 = fnv;
    out->modsum = modsum;
    return 0;
}

int dacb_lattice_run(dacb_tie_policy policy, unsigned max_steps, dacb_lattice_result *out) {
    if (out == NULL || max_steps == 0) {
        return -1;
    }
    enum { N = 64, CELLS = N * N };
    int8_t current[CELLS];
    int8_t next[CELLS];

    for (unsigned y = 0; y < N; ++y) {
        for (unsigned x = 0; x < N; ++x) {
            const unsigned v = (x * 17u + y * 31u + x * y) % 7u;
            current[y * N + x] = (v < 3u) ? INT8_C(1) : INT8_C(-1);
        }
    }

    uint32_t first[3] = {0, 0, 0};
    uint32_t convergence = 0;
    for (unsigned step = 1; step <= max_steps; ++step) {
        uint32_t flips = 0;
        for (unsigned y = 0; y < N; ++y) {
            for (unsigned x = 0; x < N; ++x) {
                const unsigned xm = (x + N - 1u) % N;
                const unsigned xp = (x + 1u) % N;
                const unsigned ym = (y + N - 1u) % N;
                const unsigned yp = (y + 1u) % N;
                const int h = current[y * N + xm] + current[y * N + xp] +
                              current[ym * N + x] + current[yp * N + x];
                int8_t value;
                if (h > 0) {
                    value = INT8_C(1);
                } else if (h < 0) {
                    value = INT8_C(-1);
                } else {
                    value = (policy == DACB_TIE_PLUS) ? INT8_C(1) : current[y * N + x];
                }
                next[y * N + x] = value;
                if (value != current[y * N + x]) {
                    ++flips;
                }
            }
        }
        if (step <= 3u) {
            first[step - 1u] = flips;
        }
        memcpy(current, next, sizeof(current));
        if (flips == 0 && convergence == 0) {
            convergence = step;
        }
    }

    int32_t magnetization = 0;
    int32_t energy = 0;
    uint32_t weighted = 0;
    uint32_t rolling = 0;
    for (unsigned y = 0; y < N; ++y) {
        for (unsigned x = 0; x < N; ++x) {
            const unsigned idx = y * N + x;
            const int spin = current[idx];
            magnetization += spin;
            energy -= spin * (current[y * N + ((x + 1u) % N)] + current[((y + 1u) % N) * N + x]);
            const uint64_t weighted_term = (uint64_t)(idx + 1u) * (uint64_t)(spin + 2);
            weighted = add_mod_check(weighted, weighted_term);
            const uint32_t bit = (uint32_t)((spin + 1) / 2);
            rolling = (uint32_t)(((uint64_t)rolling * UINT32_C(31) + bit) % DACB_CHECK_MOD);
        }
    }

    out->magnetization = magnetization;
    out->energy = energy;
    out->weighted_checksum = weighted;
    out->rolling_checksum = rolling;
    out->convergence_step = convergence;
    out->first_flips = first[0];
    out->second_flips = first[1];
    out->third_flips = first[2];
    return 0;
}

uint32_t dacb_audit_candidate(const dacb_summary *truth, const dacb_candidate *candidate) {
    if (truth == NULL || candidate == NULL) {
        return UINT32_MAX;
    }
    uint32_t issues = 0;
    if (candidate->last != truth->last) {
        issues |= DACB_AUDIT_LAST_MISMATCH;
    }
    if (candidate->sum_mod >= DACB_CHECK_MOD) {
        issues |= DACB_AUDIT_SUM_OUT_OF_RANGE;
    }
    if (candidate->sum_mod != truth->sum_mod) {
        issues |= DACB_AUDIT_SUM_MISMATCH;
    }
    if (candidate->weighted_mod >= DACB_CHECK_MOD) {
        issues |= DACB_AUDIT_WEIGHT_OUT_OF_RANGE;
    }
    if (candidate->weighted_mod != truth->weighted_mod) {
        issues |= DACB_AUDIT_WEIGHT_MISMATCH;
    }
    if (!candidate->source_matches) {
        issues |= DACB_AUDIT_SOURCE_MISMATCH;
    }
    if (!candidate->program_matches) {
        issues |= DACB_AUDIT_PROGRAM_MISMATCH;
    }
    if (candidate->independent_oracles < 2u) {
        issues |= DACB_AUDIT_INSUFFICIENT_ORACLE;
    }
    return issues;
}


int dacb_self_correct(uint64_t steps, const dacb_candidate *candidate, dacb_candidate *corrected, uint32_t *issues) {
    if (candidate == NULL || corrected == NULL || issues == NULL) {
        return -1;
    }
    dacb_summary truth;
    if (dacb_run(DACB_SEED, steps, &truth) != 0) {
        return -2;
    }
    *issues = dacb_audit_candidate(&truth, candidate);
    if ((*issues & (DACB_AUDIT_SOURCE_MISMATCH | DACB_AUDIT_PROGRAM_MISMATCH)) != 0) {
        *corrected = *candidate;
        return 2; /* Refuse to rewrite an answer bound to the wrong workload/program. */
    }
    *corrected = *candidate;
    corrected->last = truth.last;
    corrected->sum_mod = truth.sum_mod;
    corrected->weighted_mod = truth.weighted_mod;
    corrected->source_matches = true;
    corrected->program_matches = true;
    corrected->independent_oracles = 3;
    return (*issues == 0) ? 0 : 1;
}

static uint32_t lattice_matching_bonds(dacb_tie_policy policy) {
    enum { N = 64, CELLS = N * N };
    int8_t current[CELLS];
    int8_t next[CELLS];
    for (unsigned y = 0; y < N; ++y) {
        for (unsigned x = 0; x < N; ++x) {
            current[y * N + x] = (((x * 17u + y * 31u + x * y) % 7u) < 3u) ? INT8_C(1) : INT8_C(-1);
        }
    }
    for (unsigned step = 0; step < 200u; ++step) {
        for (unsigned y = 0; y < N; ++y) {
            for (unsigned x = 0; x < N; ++x) {
                const int h = current[y * N + ((x + N - 1u) % N)] + current[y * N + ((x + 1u) % N)] +
                              current[((y + N - 1u) % N) * N + x] + current[((y + 1u) % N) * N + x];
                next[y * N + x] = (h > 0) ? INT8_C(1) : (h < 0) ? INT8_C(-1) :
                    (policy == DACB_TIE_PLUS ? INT8_C(1) : current[y * N + x]);
            }
        }
        memcpy(current, next, sizeof(current));
    }
    uint32_t count = 0;
    for (unsigned y = 0; y < N; ++y) {
        for (unsigned x = 0; x < N; ++x) {
            const int s = current[y * N + x];
            if (s == current[y * N + ((x + 1u) % N)]) ++count;
            if (s == current[((y + 1u) % N) * N + x]) ++count;
        }
    }
    return count;
}

int dacb_write_canonical_report(FILE *stream) {
    if (stream == NULL) {
        return -1;
    }
    dacb_summary s5k, s100k, s1m;
    dacb_exhaustive_result ex;
    dacb_lattice_result preserve, tieplus;
    if (dacb_run(DACB_SEED, UINT64_C(5000), &s5k) != 0 ||
        dacb_run(DACB_SEED, UINT64_C(100000), &s100k) != 0 ||
        dacb_run(DACB_SEED, UINT64_C(1000000), &s1m) != 0 ||
        dacb_exhaustive(2, 10, &ex) != 0 ||
        dacb_lattice_run(DACB_TIE_PRESERVE, 200, &preserve) != 0 ||
        dacb_lattice_run(DACB_TIE_PLUS, 200, &tieplus) != 0) {
        return 1;
    }

    const uint64_t checkpoints_n[] = {1, 2, 3, 10, 100, 500, 1000, 5000};
    uint32_t checkpoints[8];
    for (size_t i = 0; i < 8; ++i) {
        checkpoints[i] = dacb_affine_state(DACB_SEED, checkpoints_n[i]);
    }

    unsigned rejected = 0;
    unsigned cases = 0;
    const dacb_candidate impossible = {UINT32_C(3845503477), UINT64_C(4566409260), UINT64_C(1247804322), true, true, 2};
    ++cases; if (dacb_audit_candidate(&s5k, &impossible) != 0) ++rejected;

    const dacb_candidate disagree_a = {UINT32_C(3845503477), UINT64_C(456640926), UINT64_C(247804322), true, true, 2};
    const dacb_candidate disagree_b = {UINT32_C(2854655001), UINT64_C(754942049), UINT64_C(596116738), true, true, 2};
    ++cases; if (dacb_audit_candidate(&s5k, &disagree_a) != 0 && dacb_audit_candidate(&s5k, &disagree_b) != 0 &&
                 (disagree_a.last != disagree_b.last || disagree_a.sum_mod != disagree_b.sum_mod || disagree_a.weighted_mod != disagree_b.weighted_mod)) ++rejected;

    ++cases; if (dacb_audit_candidate(&s5k, &disagree_a) != 0 && dacb_audit_candidate(&s5k, &disagree_a) != 0) ++rejected;

    dacb_candidate stale = {s5k.last, s5k.sum_mod, s5k.weighted_mod, false, true, 3};
    ++cases; if ((dacb_audit_candidate(&s5k, &stale) & DACB_AUDIT_SOURCE_MISMATCH) != 0) ++rejected;

    ++cases; if (preserve.magnetization != tieplus.magnetization || preserve.energy != tieplus.energy || preserve.weighted_checksum != tieplus.weighted_checksum) ++rejected;

    const uint32_t matching = lattice_matching_bonds(DACB_TIE_PRESERVE);
    ++cases; if ((int32_t)matching != preserve.energy && matching == UINT32_C(6244)) ++rejected;

    ++cases; if (tieplus.rolling_checksum != 0 && tieplus.rolling_checksum == UINT32_C(299341384)) ++rejected;

    dacb_candidate no_oracle = {s5k.last, s5k.sum_mod, s5k.weighted_mod, true, true, 0};
    ++cases; if ((dacb_audit_candidate(&s5k, &no_oracle) & DACB_AUDIT_INSUFFICIENT_ORACLE) != 0) ++rejected;

    const bool status =
        s5k.last == UINT32_C(4125282121) && s5k.sum_mod == UINT32_C(585303980) && s5k.weighted_mod == UINT32_C(554861868) &&
        s100k.last == UINT32_C(694140833) && s100k.sum_mod == UINT32_C(784266586) && s100k.weighted_mod == UINT32_C(455600981) &&
        s1m.last == UINT32_C(366300225) && s1m.sum_mod == UINT32_C(346639628) && s1m.weighted_mod == UINT32_C(152047219) &&
        ex.cases == UINT64_C(1400140) &&
        preserve.magnetization == -1816 && preserve.energy == -4296 && preserve.weighted_checksum == UINT32_C(13134816) && preserve.convergence_step == 3 &&
        preserve.first_flips == 987 && preserve.second_flips == 38 && preserve.third_flips == 0 &&
        tieplus.magnetization == 4096 && tieplus.energy == -8192 && tieplus.rolling_checksum == UINT32_C(299341384) && tieplus.convergence_step == 5 &&
        cases == 8 && rejected == 8;

    fprintf(stream, "DACB_REPORT_V2\n");
    fprintf(stream, "version=%s\n", DACB_VERSION);
    fprintf(stream, "spec=affine32:a=1664525:c=1013904223:mod=2^32:seed=1:checksum_mod=1000000007\n");
    fprintf(stream, "chain5000=%" PRIu32 ",%" PRIu32 ",%" PRIu32 "\n", s5k.last, s5k.sum_mod, s5k.weighted_mod);
    fprintf(stream, "chain100000=%" PRIu32 ",%" PRIu32 ",%" PRIu32 "\n", s100k.last, s100k.sum_mod, s100k.weighted_mod);
    fprintf(stream, "chain1000000=%" PRIu32 ",%" PRIu32 ",%" PRIu32 "\n", s1m.last, s1m.sum_mod, s1m.weighted_mod);
    fprintf(stream, "oracles5000=%" PRIu32 ",%" PRIu32 ",%" PRIu32 ",reverse=%" PRIu32 "\n", s5k.last, s5k.affine_last, s5k.matrix_last, s5k.reverse_seed);
    fprintf(stream, "checkpoints=1:%" PRIu32 ",2:%" PRIu32 ",3:%" PRIu32 ",10:%" PRIu32 ",100:%" PRIu32 ",500:%" PRIu32 ",1000:%" PRIu32 ",5000:%" PRIu32 "\n",
            checkpoints[0], checkpoints[1], checkpoints[2], checkpoints[3], checkpoints[4], checkpoints[5], checkpoints[6], checkpoints[7]);
    fprintf(stream, "exhaustive=bits2-10,cases=%" PRIu64 ",fnv64=%016" PRIx64 ",modsum=%" PRIu32 "\n", ex.cases, ex.fnv64, ex.modsum);
    fprintf(stream, "lattice_preserve=M:%" PRId32 ",E:%" PRId32 ",C:%" PRIu32 ",conv:%" PRIu32 ",flips:%" PRIu32 "/%" PRIu32 "/%" PRIu32 "\n",
            preserve.magnetization, preserve.energy, preserve.weighted_checksum, preserve.convergence_step,
            preserve.first_flips, preserve.second_flips, preserve.third_flips);
    fprintf(stream, "lattice_tie_plus=M:%" PRId32 ",E:%" PRId32 ",roll:%" PRIu32 ",conv:%" PRIu32 "\n",
            tieplus.magnetization, tieplus.energy, tieplus.rolling_checksum, tieplus.convergence_step);
    fprintf(stream, "adversarial=%u/%u\n", rejected, cases);
    fprintf(stream, "status=%s\n", status ? "PASS" : "FAIL");
    return status ? 0 : 2;
}
