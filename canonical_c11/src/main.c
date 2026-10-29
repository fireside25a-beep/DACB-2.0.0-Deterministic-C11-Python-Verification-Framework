#include "dacb.h"

#include <errno.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int parse_u64(const char *text, uint64_t *out) {
    if (*text == '\0') return -1;
    for (const char *p = text; *p != '\0'; ++p) {
        if (*p < '0' || *p > '9') return -1;
    }
    char *end = NULL;
    errno = 0;
    const unsigned long long v = strtoull(text, &end, 10);
    if (errno != 0 || end == text || *end != '\0' || v > UINT64_MAX) return -1;
    *out = (uint64_t)v;
    return 0;
}

static void usage(const char *argv0) {
    fprintf(stderr, "usage: %s report | run STEPS | exhaustive [MIN_BITS MAX_BITS] | lattice preserve|plus\n", argv0);
}

int main(int argc, char **argv) {
    if (argc < 2) {
        usage(argv[0]);
        return 2;
    }
    if (strcmp(argv[1], "report") == 0 && argc == 2) {
        return dacb_write_canonical_report(stdout);
    }
    if (strcmp(argv[1], "run") == 0 && argc == 3) {
        uint64_t steps;
        if (parse_u64(argv[2], &steps) != 0) {
            fprintf(stderr, "invalid step count\n");
            return 2;
        }
        dacb_summary s;
        if (dacb_run(DACB_SEED, steps, &s) != 0) return 1;
        printf("steps=%" PRIu64 " seed=%" PRIu32 " last=%" PRIu32 " affine=%" PRIu32 " matrix=%" PRIu32
               " reverse=%" PRIu32 " sum_mod=%" PRIu32 " weighted_mod=%" PRIu32 "\n",
               s.steps, s.seed, s.last, s.affine_last, s.matrix_last, s.reverse_seed, s.sum_mod, s.weighted_mod);
        return 0;
    }
    if (strcmp(argv[1], "exhaustive") == 0 && (argc == 2 || argc == 4)) {
        unsigned lo = 2, hi = 10;
        if (argc == 4) {
            uint64_t l, h;
            if (parse_u64(argv[2], &l) != 0 || parse_u64(argv[3], &h) != 0 || l > 16 || h > 16) return 2;
            lo = (unsigned)l; hi = (unsigned)h;
        }
        dacb_exhaustive_result e;
        const int rc = dacb_exhaustive(lo, hi, &e);
        if (rc != 0) return rc < 0 ? 2 : 1;
        printf("bits=%u-%u cases=%" PRIu64 " fnv64=%016" PRIx64 " modsum=%" PRIu32 "\n", lo, hi, e.cases, e.fnv64, e.modsum);
        return 0;
    }
    if (strcmp(argv[1], "lattice") == 0 && argc == 3) {
        dacb_tie_policy p;
        if (strcmp(argv[2], "preserve") == 0) p = DACB_TIE_PRESERVE;
        else if (strcmp(argv[2], "plus") == 0) p = DACB_TIE_PLUS;
        else return 2;
        dacb_lattice_result r;
        if (dacb_lattice_run(p, 200, &r) != 0) return 1;
        printf("M=%" PRId32 " E=%" PRId32 " weighted=%" PRIu32 " rolling=%" PRIu32 " convergence=%" PRIu32 " flips=%" PRIu32 ",%" PRIu32 ",%" PRIu32 "\n",
               r.magnetization, r.energy, r.weighted_checksum, r.rolling_checksum, r.convergence_step,
               r.first_flips, r.second_flips, r.third_flips);
        return 0;
    }
    usage(argv[0]);
    return 2;
}
