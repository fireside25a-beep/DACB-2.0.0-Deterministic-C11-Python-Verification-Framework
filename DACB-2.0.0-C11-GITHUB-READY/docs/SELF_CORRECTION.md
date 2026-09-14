# Self-correction module

The self-correction module is deliberately narrower than a general repair engine.

Given a candidate tuple for a known DACB workload, the module:

1. executes the canonical C11 recurrence;
2. verifies the result against affine and matrix oracles;
3. checks modulo bounds;
4. checks source/workload identity;
5. checks program/semantic identity;
6. either accepts, corrects the numeric tuple, or refuses correction.

A candidate bound to a stale source or semantically changed program is never "fixed" by copying the current canonical answer. The function returns refusal so the mismatch remains visible.

This distinction prevents self-correction from hiding state/history bugs.
