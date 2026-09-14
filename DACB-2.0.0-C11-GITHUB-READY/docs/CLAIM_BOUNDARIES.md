# Claim boundaries

DACB 2.0.0 supports the following claims when the included release verification passes:

- the C11 recurrence is deterministic under standard unsigned-integer semantics;
- the recorded known vectors reproduce;
- direct, affine, matrix, and inverse state checks agree on tested workloads;
- the documented reduced finite domains are exhaustively checked;
- GCC, Clang, and the independent Python twin produce identical canonical output bytes for the stated gates;
- the listed adversarial mutations are detected or rejected;
- the C tests pass under the stated sanitizer/analyzer runs.

DACB does **not** claim:

- exhaustive enumeration of the entire `2^32 x 2^32` seed/step space;
- cryptographic security or randomness quality;
- that a checksum collision is impossible;
- that byte-identical reports prove byte-identical machine code;
- deterministic wall-clock timing across machines;
- that a language model response was produced by local code execution unless the execution environment is independently established;
- that the lattice regression is a physical phase-transition benchmark.
