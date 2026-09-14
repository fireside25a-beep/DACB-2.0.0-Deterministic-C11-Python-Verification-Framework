# Lumus Maxima regression case

This document records the behavioral cases that motivated DACB 2.0.0's adversarial module. It is a regression record, not evidence about hidden provider infrastructure or locality of execution.

## Affine-chain probe

Canonical input semantics:

```text
x0 = 1
x[n+1] = (1664525*x[n] + 1013904223) mod 2^32
5000 steps
```

Verified canonical tuple:

```text
5000, 4125282121, 585303980, 554861868
```

Captured candidate tuples included:

```text
3845503477, 4566409260, 1247804322
2854655001, 754942049, 596116738
```

The first candidate is immediately impossible because both remainder fields exceed the stated checksum modulus `1,000,000,007`. Repetition or agreement with another candidate does not make it valid.

## Stale code-retrieval regression

After newer affine, Poisson, and wave inputs, a request to show the current code repeatedly surfaced an older N=64 spin workload. DACB models this class of error as **workload/source mismatch**. A numerically correct answer is still rejected if it is attached to the wrong workload identity.

## Silent semantic mutation regression

The preserved spin rule is:

```text
positive neighbor sum -> +1
negative neighbor sum -> -1
tie                  -> preserve current spin
```

A later generated variant changed a tie into `+1`. That change drives the system to a different terminal state. DACB keeps both implementations only as a positive/negative semantic regression pair and never treats them as equivalent.

## Metric-definition regression

One generated variant changed the meaning of the energy field from nearest-neighbor bond-product energy to a count of matching bonds. DACB assigns those metrics different semantics and rejects silent relabeling.

## Checksum-scope regression

A generated checksum expression using assignment inside a comprehension left the outer checksum unchanged in the tested form. DACB independently recomputes the expected rolling checksum and rejects the zero result.

## Verification boundary

The case supports testing for:

- deterministic arithmetic errors;
- impossible remainders;
- provider disagreement;
- copied wrong consensus;
- stale workload retrieval;
- silent source-semantic mutation;
- metric-definition drift;
- claims of verification without an independent oracle.

It does not by itself establish whether a provider response came from local execution, remote execution, manual-style reasoning, or another internal path. DACB deliberately tests only observable correctness and consistency.
