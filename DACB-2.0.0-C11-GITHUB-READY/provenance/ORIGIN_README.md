# DACB 2.0.0 — Deterministic Affine Chain Benchmark

DACB is a compact verification framework for long deterministic dependency chains. The canonical implementation is **portable C11**. A separately written **Python digital twin** implements the same published mathematics without calling, importing, parsing, or sharing runtime state with the C implementation.

The core question is simple:

> Can an implementation preserve the program, execute the complete dependency chain, respect exact arithmetic bounds, agree with independent mathematical oracles, detect semantic drift, and reject a confidently repeated wrong answer?

DACB does not score prose quality and does not treat agreement between two answers as proof. Correctness is established by independent computation.

## Architecture

```text
published affine recurrence specification
          |
          +--> canonical_c11/        canonical implementation
          |      sequential chain
          |      affine exponentiation oracle
          |      2x2 matrix oracle
          |      reverse/inverse oracle
          |      self-correction + adversarial audit
          |
          +--> python_twin/          independent digital twin
                 independent source and runtime path

C11 report --cmp--> Python report
        exact byte-for-byte identity required
```

The C11 and Python implementations do not invoke one another. Cross-language comparison occurs only after each program independently produces its output bytes.

## Canonical recurrence

```text
x[n+1] = (1664525*x[n] + 1013904223) mod 2^32
x[0]   = 1
```

The canonical 5,000-step vector is:

```text
last          = 4125282121
sum_mod       = 585303980
weighted_mod  = 554861868
```

The three forward oracles and the reverse oracle all return the same state/seed relation.

## Verification axes

DACB deliberately tests different failure directions instead of relying on one large run:

- **Depth axis:** exact chains through 1,000,000 dependent transitions.
- **State-space axis:** every seed and every step through complete reduced power-of-two periods.
- **Oracle axis:** sequential iteration, affine-transform exponentiation, homogeneous 2x2 matrix exponentiation, and inverse traversal.
- **Implementation axis:** GCC C11, Clang C11, and the independent Python digital twin.
- **Byte-identity axis:** canonical reports and selected CLI results must be byte-for-byte identical.
- **Numeric axis:** unsigned wrap semantics, checksum-range constraints, inverse arithmetic, exact finite dyadic duplicate forms, and boundary seeds.
- **Mutation axis:** deliberately wrong multiplier, increment, weighting, wrap behavior, tie semantics, metric definitions, and checksum behavior must be detected.
- **Recovery axis:** incorrect numeric candidates can be corrected only from independent internal oracles; stale or semantically different workloads are refused rather than silently rewritten.

## Bounded exhaustive campaigns

The fast canonical report exhausts powers `2^2 .. 2^10`:

```text
1,400,140 seed/step cases
```

The deep cross-language gate exhausts `2^2 .. 2^11` independently in C11 and Python:

```text
5,596,492 seed/step cases
```

The extended C11 gate exhausts `2^2 .. 2^12`:

```text
22,377,804 seed/step cases
```

For the production `2^32` recurrence DACB uses exact algebraic oracles and the standard full-period conditions for a power-of-two modulus; it does **not** claim to enumerate all `2^32` seeds across a complete period.

## Adversarial regression module

The non-core lattice regression module preserves the exact original tie rule and detects silent semantic changes. It covers:

- impossible modulo outputs;
- disagreement between candidate answers;
- repeated wrong consensus;
- stale workload/source retrieval;
- silent mutation of tie semantics;
- changing an energy definition while retaining the same label;
- checksum-scope defects;
- claims of verification without independent oracles.

The lattice module is a semantic-drift regression fixture. It is not part of the affine benchmark's mathematical core.

## Build

Requirements:

- ISO C11 compiler (GCC or Clang)
- Python 3.10+ only for the independent twin and cross-language gate
- POSIX shell utilities for the release verifier

Build the canonical C11 tool:

```sh
make
```

Run the C tests plus the independent twin tests:

```sh
make test
```

Generate the canonical C report:

```sh
./build/dacb report
```

Generate the independent Python report:

```sh
python3 python_twin/dacb_twin.py report
```

Require exact report identity:

```sh
make cross
```

Run the complete release verification campaign:

```sh
bash scripts/verify_release.sh
```

## CLI

```text
dacb report
dacb run STEPS
dacb exhaustive [MIN_BITS MAX_BITS]
dacb lattice preserve
dacb lattice plus
```

The `lattice plus` command intentionally evaluates the mutated tie-to-`+1` rule so the semantic regression can prove that it differs from the preserved rule.

## Repository layout

```text
canonical_c11/       canonical C11 library + CLI
python_twin/         independently written Python digital twin
tests/               C tests, twin tests, adversarial fixtures
scripts/             release verifier and optional timing utility
docs/                mathematics, architecture, verification and boundaries
evidence/            sealed release verification records
.github/workflows/    CI
```

No generated binaries, object files, Python caches, or temporary build directories are included in the release archive.

## What DACB is not

DACB is not a cryptographic random-number benchmark, not a randomness-quality claim, not a physics solver, and not evidence that agreement by multiple models implies correctness. The recurrence is intentionally simple enough to admit multiple exact oracles while still producing long dependency chains whose final values are not convenient to infer by inspection.

See `docs/MATH.md`, `docs/ARCHITECTURE.md`, `docs/DIGITAL_TWIN.md`, `docs/SELF_CORRECTION.md`, `docs/VERIFICATION.md`, `docs/ADVERSARIAL.md`, `docs/REPORT_FORMAT.md`, `docs/LUMUS_REGRESSION_CASE.md`, and `docs/CLAIM_BOUNDARIES.md` for the formal contract and regression record.
