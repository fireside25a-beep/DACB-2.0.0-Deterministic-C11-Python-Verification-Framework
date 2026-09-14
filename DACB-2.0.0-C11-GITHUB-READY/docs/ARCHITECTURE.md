# Architecture

## 1. Canonical implementation boundary

`canonical_c11/` is the authoritative implementation of DACB 2.0.0.

It contains no Python embedding, no FFI, no subprocess calls, no network path, and no dependency on generated Python data. The arithmetic is expressed with fixed-width unsigned integers and explicit modulo checks.

The canonical C11 path has four independent state checks:

1. direct sequential transition;
2. binary exponentiation of an affine transform;
3. binary exponentiation of a 2x2 homogeneous matrix;
4. inverse traversal back to the starting seed.

The first three must agree on the final state. The fourth must reconstruct the original seed.

## 2. Independent digital twin

`python_twin/dacb_twin.py` is intentionally separate.

It does not:

- import a C extension;
- run the C executable;
- read the C report;
- read a generated vector file;
- call a shared project library;
- obtain state from a subprocess.

It independently implements the published recurrence, affine composition, matrix algebra, inverse step, checksums, reduced-domain exhaustion, semantic regression, and adversarial audit.

The implementations therefore share **specification**, not implementation state.

## 3. Byte identity contract

Both implementations emit a canonical ASCII report with fixed ordering and formatting. The release verifier compares the files using `cmp`.

This is stronger than comparing parsed fields: any character, spacing, line-order, newline, numeric, or status divergence fails the gate.

GCC and Clang builds of the C11 implementation must also emit identical report bytes.

## 4. Self-correction boundary

`dacb_self_correct()` is not a guesser. It first recomputes the truth using the canonical chain and internal independent oracles.

A candidate can be numerically corrected only when its workload/source identity and program semantics still match the canonical workload. If either identity check fails, correction is refused. This prevents a stale answer from being "corrected" into a result for a different program.

The correction state is therefore one of:

```text
already valid
corrected from canonical oracles
refused because workload/program identity changed
```

## 5. Semantic-regression boundary

The N=64 spin system is not merged into the affine core. It lives as an adversarial semantic regression because it exposes a different class of failure: source text can look similar while changing the update rule or metric definition.

Canonical tie behavior:

```text
h > 0  -> +1
h < 0  -> -1
h == 0 -> preserve current spin
```

The mutation fixture intentionally changes the tie to `+1`. The two systems converge to different states and fingerprints, so the mutation is detectable.
