# Adversarial regression suite

The adversarial module tests verification behavior rather than the affine recurrence alone.

## 1. Impossible modulo result

A candidate whose `sum_mod` or `weighted_mod` is greater than or equal to `1,000,000,007` is impossible under the stated expression and is rejected before any consensus logic.

## 2. Provider disagreement

Two different candidate tuples are not averaged, voted, or accepted. Each is independently compared to the canonical oracles.

## 3. Copied wrong consensus

Repeating the same wrong tuple does not increase its authority. Consensus without oracle agreement remains a failure.

## 4. Stale workload retrieval

A numerically correct tuple attached to a mismatching workload/source fingerprint is rejected. Self-correction refuses to rewrite it because the system may be evaluating a different program.

## 5. Silent tie mutation

The preserved N=64 update rule leaves ties unchanged. The mutation changes ties to `+1`.

Preserved rule after convergence:

```text
M = -1816
E = -4296
weighted checksum = 13134816
flip trajectory = 987, 38, 0
convergence step = 3
```

Tie-to-plus mutation:

```text
M = 4096
E = -8192
rolling checksum = 299341384
convergence step = 5
```

The difference is intentional evidence that superficially similar code can implement a different state machine.

## 6. Energy-definition drift

For the preserved lattice, conventional nearest-neighbor bond-product energy is `-4296`. Counting matching bonds instead produces `6244`. DACB treats these as different metrics and rejects silent relabeling.

## 7. Checksum-scope defect

A regression candidate that leaves the rolling checksum at zero is rejected. The independently recomputed checksum for the mutated all-plus terminal lattice is `299341384`.

## 8. Verification without an oracle

Even an accidentally correct tuple is not labelled verified when the candidate declares no independent oracle. The audit module records insufficient verification.

## Self-correction rule

Numerical self-correction is allowed only when workload/source and program identity still match. The corrected tuple is generated from the canonical sequential result plus affine and matrix oracle agreement. Wrong-workload cases are refused instead of silently rewritten.
