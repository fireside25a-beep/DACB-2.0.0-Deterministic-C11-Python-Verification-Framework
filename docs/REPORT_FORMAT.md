# Canonical report format

`dacb report` and `python_twin/dacb_twin.py report` emit ASCII with LF line endings only.

The field order is fixed:

```text
DACB_REPORT_V2
version=...
spec=...
chain5000=last,sum_mod,weighted_mod
chain100000=last,sum_mod,weighted_mod
chain1000000=last,sum_mod,weighted_mod
oracles5000=sequential,affine,matrix,reverse=seed
checkpoints=step:value,...
exhaustive=bits2-10,cases=...,fnv64=...,modsum=...
lattice_preserve=...
lattice_tie_plus=...
adversarial=rejected/total
status=PASS|FAIL
```

No timestamps, hostnames, PIDs, locale-sensitive formatting, floating-point values, compiler names, or environment-derived fields are permitted in the canonical report.

This allows exact byte comparison across compilers and languages.
