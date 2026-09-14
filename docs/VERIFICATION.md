# Verification methodology

DACB separates correctness claims by axis so that one passing check cannot silently stand in for another.

## A. Known-vector tests

Exact checkpoints are verified at:

```text
1, 2, 3, 10, 100, 500, 1000, 5000, 100000, 1000000 steps
```

The 5,000-step canonical tuple is:

```text
4125282121, 585303980, 554861868
```

The 1,000,000-step tuple is:

```text
366300225, 346639628, 152047219
```

## B. Oracle agreement

For selected and exhaustive domains:

```text
sequential == affine exponentiation == matrix exponentiation
```

and reverse traversal must return the original seed.

Composition laws are checked for many pairs `(m,n)`:

```text
T^(m+n) == T^m o T^n
M^(m+n) == M^m M^n
```

## C. Reduced-domain exhaustion

For each tested power-of-two modulus:

- every seed is enumerated;
- every step `0..period` is enumerated;
- the sequential state is compared to both algebraic oracles;
- no seed may return early;
- every seed must return exactly at one complete period;
- every case contributes to deterministic FNV-1a and modular campaign fingerprints.

Campaign sizes:

```text
2..10 :  1,400,140 cases  (canonical report)
2..11 :  5,596,492 cases  (C/Python cross-language deep gate)
2..12 : 22,377,804 cases  (extended C gate)
```

## D. Boundary and numeric tests

The suite covers:

- seed `0`;
- seed `1`;
- seed `UINT32_MAX`;
- zero-step identity;
- forward/inverse round trips;
- multiplication values that exceed 32 bits before reduction;
- modulo-output range checks;
- exact finite dyadic duplicate representations;
- invalid CLI arguments.

## E. Mutation tests

The suite intentionally introduces:

- multiplier `a+2`;
- increment `c+2`;
- off-by-one weighting;
- omitted 32-bit reduction;
- tie-preserve -> tie-plus semantic drift;
- energy-definition drift;
- checksum-scope defect.

Each mutation must differ from the canonical result or be explicitly rejected by the audit module.

## F. Cross-implementation identity

The release verifier builds C with GCC and Clang, then runs the independent Python twin. It requires byte-for-byte identity for:

- canonical report;
- 5,000-step CLI result;
- 1,000,000-step CLI result;
- reduced exhaustive `2..11` summary.

## G. Compiler and runtime checks

The release gate uses:

- GCC strict C11 build;
- Clang strict C11 build;
- `-Wall -Wextra -Werror -pedantic`;
- Clang AddressSanitizer + UndefinedBehaviorSanitizer;
- GCC `-fanalyzer`;
- repeated report generation;
- source-tree artifact hygiene.

Wall-clock timing is intentionally excluded from correctness claims.
