# Contributing

Changes to DACB must preserve the distinction between specification, implementation, evidence, and presentation.

A pull request that changes arithmetic semantics must:

1. state the semantic change explicitly;
2. add or update an independent oracle;
3. update known vectors only after the old vectors fail for the documented reason;
4. add a regression case for the previous behavior;
5. pass C11 tests, Python-twin tests, cross-language byte identity, exhaustive reduced-domain gates, sanitizers, analyzer, and source-tree hygiene.

Do not weaken a failing test merely to make a changed implementation pass.
