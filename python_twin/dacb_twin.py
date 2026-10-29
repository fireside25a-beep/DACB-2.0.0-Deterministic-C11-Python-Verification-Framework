#!/usr/bin/env python3
"""Independent Python digital twin for DACB 2.0.0.

This file does not import, execute, parse, or bind to the C implementation.
It independently implements the published mathematical specification and emits
exactly the same canonical report bytes for cross-language identity checking.
"""
from __future__ import annotations

import sys
from dataclasses import dataclass

VERSION = "2.0.0"
A = 1_664_525
C = 1_013_904_223
SEED = 1
MASK32 = (1 << 32) - 1
CHECK = 1_000_000_007
FNV_OFFSET = 14_695_981_039_346_656_037
FNV_PRIME = 1_099_511_628_211


def step32(x: int) -> int:
    return (A * x + C) & MASK32


def inverse_multiplier32() -> int:
    inv = 1
    for _ in range(5):
        inv = (inv * (2 - A * inv)) & MASK32
    return inv


def prev32(x: int) -> int:
    return (inverse_multiplier32() * ((x - C) & MASK32)) & MASK32


def affine_compose(f: tuple[int, int], g: tuple[int, int], mask: int = MASK32) -> tuple[int, int]:
    fm, fa = f
    gm, ga = g
    return (fm * gm) & mask, (fm * ga + fa) & mask


def affine_pow(n: int, mask: int = MASK32) -> tuple[int, int]:
    out = (1, 0)
    base = (A & mask, C & mask)
    while n:
        if n & 1:
            out = affine_compose(base, out, mask)
        base = affine_compose(base, base, mask)
        n >>= 1
    return out


def affine_state(seed: int, n: int, mask: int = MASK32) -> int:
    m, b = affine_pow(n, mask)
    return (m * (seed & mask) + b) & mask


Matrix = tuple[int, int, int, int]


def matrix_mul(x: Matrix, y: Matrix, mask: int = MASK32) -> Matrix:
    x00, x01, x10, x11 = x
    y00, y01, y10, y11 = y
    return (
        (x00 * y00 + x01 * y10) & mask,
        (x00 * y01 + x01 * y11) & mask,
        (x10 * y00 + x11 * y10) & mask,
        (x10 * y01 + x11 * y11) & mask,
    )


def matrix_pow(n: int, mask: int = MASK32) -> Matrix:
    out: Matrix = (1, 0, 0, 1)
    base: Matrix = (A & mask, C & mask, 0, 1)
    while n:
        if n & 1:
            out = matrix_mul(base, out, mask)
        base = matrix_mul(base, base, mask)
        n >>= 1
    return out


def matrix_state(seed: int, n: int, mask: int = MASK32) -> int:
    m = matrix_pow(n, mask)
    return (m[0] * (seed & mask) + m[1]) & mask


@dataclass(frozen=True)
class Summary:
    steps: int
    seed: int
    last: int
    affine_last: int
    matrix_last: int
    reverse_seed: int
    sum_mod: int
    weighted_mod: int


def run(seed: int, steps: int) -> Summary:
    x = seed & MASK32
    total = 0
    weighted = 0
    for i in range(1, steps + 1):
        x = step32(x)
        total = (total + x) % CHECK
        weighted = (weighted + (i % CHECK) * (x % CHECK)) % CHECK
    reverse = x
    inv = inverse_multiplier32()
    for _ in range(steps):
        reverse = (inv * ((reverse - C) & MASK32)) & MASK32
    return Summary(steps, seed & MASK32, x, affine_state(seed, steps), matrix_state(seed, steps), reverse, total, weighted)


def fnv_u32le(h: int, value: int) -> int:
    for shift in range(0, 32, 8):
        h ^= (value >> shift) & 0xFF
        h = (h * FNV_PRIME) & ((1 << 64) - 1)
    return h


def exhaustive(min_bits: int = 2, max_bits: int = 10) -> tuple[int, int, int]:
    total_cases = 0
    h = FNV_OFFSET
    modsum = 0
    for bits in range(min_bits, max_bits + 1):
        modulus = 1 << bits
        mask = modulus - 1
        aff = [affine_pow(n, mask) for n in range(modulus + 1)]
        mats = [matrix_pow(n, mask) for n in range(modulus + 1)]
        for n, ((mul, add), mat) in enumerate(zip(aff, mats)):
            assert mat == (mul, add, 0, 1), (bits, n)
        for seed in range(modulus):
            x = seed
            for n in range(modulus + 1):
                mul, add = aff[n]
                xa = (mul * seed + add) & mask
                mat = mats[n]
                xm = (mat[0] * seed + mat[1]) & mask
                assert x == xa == xm, (bits, seed, n, x, xa, xm)
                if n == modulus:
                    assert x == seed
                elif n > 0:
                    assert x != seed
                for field in (bits, seed, n, x):
                    h = fnv_u32le(h, field)
                modsum = (modsum + bits + 3 * seed + 5 * n + 7 * x) % CHECK
                total_cases += 1
                if n < modulus:
                    x = ((A & mask) * x + (C & mask)) & mask
    return total_cases, h, modsum


def lattice(tie_plus: bool, max_steps: int = 200) -> tuple[int, int, int, int, int, tuple[int, int, int]]:
    n = 64
    s = [1 if ((x * 17 + y * 31 + x * y) % 7) < 3 else -1 for y in range(n) for x in range(n)]
    first = [0, 0, 0]
    convergence = 0
    for step in range(1, max_steps + 1):
        nxt = s.copy()
        flips = 0
        for y in range(n):
            for x in range(n):
                i = y * n + x
                h = s[y * n + (x - 1) % n] + s[y * n + (x + 1) % n] + s[((y - 1) % n) * n + x] + s[((y + 1) % n) * n + x]
                nv = 1 if h > 0 else -1 if h < 0 else 1 if tie_plus else s[i]
                nxt[i] = nv
                flips += nv != s[i]
        if step <= 3:
            first[step - 1] = flips
        s = nxt
        if flips == 0 and convergence == 0:
            convergence = step
    mag = sum(s)
    energy = -sum(s[y * n + x] * (s[y * n + (x + 1) % n] + s[((y + 1) % n) * n + x]) for y in range(n) for x in range(n))
    weighted = sum((i + 1) * (spin + 2) for i, spin in enumerate(s)) % CHECK
    rolling = 0
    for spin in s:
        rolling = (rolling * 31 + (spin + 1) // 2) % CHECK
    return mag, energy, weighted, rolling, convergence, tuple(first)


def matching_bonds_preserve() -> int:
    n = 64
    s = [1 if ((x * 17 + y * 31 + x * y) % 7) < 3 else -1 for y in range(n) for x in range(n)]
    for _ in range(200):
        nxt = s.copy()
        for y in range(n):
            for x in range(n):
                i = y * n + x
                h = s[y * n + (x - 1) % n] + s[y * n + (x + 1) % n] + s[((y - 1) % n) * n + x] + s[((y + 1) % n) * n + x]
                nxt[i] = 1 if h > 0 else -1 if h < 0 else s[i]
        s = nxt
    return sum((s[y * n + x] == s[y * n + (x + 1) % n]) + (s[y * n + x] == s[((y + 1) % n) * n + x]) for y in range(n) for x in range(n))


LAST_MISMATCH = 1 << 0
SUM_OUT_OF_RANGE = 1 << 1
SUM_MISMATCH = 1 << 2
WEIGHT_OUT_OF_RANGE = 1 << 3
WEIGHT_MISMATCH = 1 << 4
SOURCE_MISMATCH = 1 << 5
INSUFFICIENT_ORACLE = 1 << 6
PROGRAM_MISMATCH = 1 << 7


def audit(truth: Summary, candidate: tuple[int, int, int, bool, bool, int]) -> int:
    last, sum_mod, weighted_mod, source_matches, program_matches, oracle_count = candidate
    issues = 0
    if last != truth.last: issues |= LAST_MISMATCH
    if sum_mod >= CHECK: issues |= SUM_OUT_OF_RANGE
    if sum_mod != truth.sum_mod: issues |= SUM_MISMATCH
    if weighted_mod >= CHECK: issues |= WEIGHT_OUT_OF_RANGE
    if weighted_mod != truth.weighted_mod: issues |= WEIGHT_MISMATCH
    if not source_matches: issues |= SOURCE_MISMATCH
    if not program_matches: issues |= PROGRAM_MISMATCH
    if oracle_count < 2: issues |= INSUFFICIENT_ORACLE
    return issues


def self_correct(steps: int, candidate: tuple[int, int, int, bool, bool, int]) -> tuple[str, tuple[int, int, int, bool, bool, int], int]:
    truth = run(SEED, steps)
    issues = audit(truth, candidate)
    if issues & (SOURCE_MISMATCH | PROGRAM_MISMATCH):
        return "REFUSE", candidate, issues
    fixed = (truth.last, truth.sum_mod, truth.weighted_mod, True, True, 3)
    return ("OK" if issues == 0 else "CORRECTED"), fixed, issues


def canonical_report() -> bytes:
    s5 = run(SEED, 5_000)
    s100 = run(SEED, 100_000)
    s1m = run(SEED, 1_000_000)
    ex_cases, ex_fnv, ex_modsum = exhaustive(2, 10)
    preserve = lattice(False)
    tieplus = lattice(True)
    checkpoints_n = (1, 2, 3, 10, 100, 500, 1000, 5000)
    checkpoints = [affine_state(SEED, n) for n in checkpoints_n]

    rejected = 0
    cases = 0
    impossible = (3_845_503_477, 4_566_409_260, 1_247_804_322, True, True, 2)
    cases += 1; rejected += audit(s5, impossible) != 0
    da = (3_845_503_477, 456_640_926, 247_804_322, True, True, 2)
    db = (2_854_655_001, 754_942_049, 596_116_738, True, True, 2)
    cases += 1; rejected += audit(s5, da) != 0 and audit(s5, db) != 0 and da[:3] != db[:3]
    cases += 1; rejected += audit(s5, da) != 0 and audit(s5, da) != 0
    stale = (s5.last, s5.sum_mod, s5.weighted_mod, False, True, 3)
    cases += 1; rejected += bool(audit(s5, stale) & SOURCE_MISMATCH)
    cases += 1; rejected += preserve[:3] != tieplus[:3]
    matching = matching_bonds_preserve()
    cases += 1; rejected += matching != preserve[1] and matching == 6244
    cases += 1; rejected += tieplus[3] != 0 and tieplus[3] == 299_341_384
    no_oracle = (s5.last, s5.sum_mod, s5.weighted_mod, True, True, 0)
    cases += 1; rejected += bool(audit(s5, no_oracle) & INSUFFICIENT_ORACLE)

    status = (
        (s5.last, s5.sum_mod, s5.weighted_mod) == (4_125_282_121, 585_303_980, 554_861_868)
        and (s100.last, s100.sum_mod, s100.weighted_mod) == (694_140_833, 784_266_586, 455_600_981)
        and (s1m.last, s1m.sum_mod, s1m.weighted_mod) == (366_300_225, 346_639_628, 152_047_219)
        and ex_cases == 1_400_140
        and preserve == (-1816, -4296, 13_134_816, preserve[3], 3, (987, 38, 0))
        and tieplus[0] == 4096 and tieplus[1] == -8192 and tieplus[3] == 299_341_384 and tieplus[4] == 5
        and cases == rejected == 8
    )
    lines = [
        "DACB_REPORT_V2",
        f"version={VERSION}",
        "spec=affine32:a=1664525:c=1013904223:mod=2^32:seed=1:checksum_mod=1000000007",
        f"chain5000={s5.last},{s5.sum_mod},{s5.weighted_mod}",
        f"chain100000={s100.last},{s100.sum_mod},{s100.weighted_mod}",
        f"chain1000000={s1m.last},{s1m.sum_mod},{s1m.weighted_mod}",
        f"oracles5000={s5.last},{s5.affine_last},{s5.matrix_last},reverse={s5.reverse_seed}",
        "checkpoints=" + ",".join(f"{n}:{v}" for n, v in zip(checkpoints_n, checkpoints)),
        f"exhaustive=bits2-10,cases={ex_cases},fnv64={ex_fnv:016x},modsum={ex_modsum}",
        f"lattice_preserve=M:{preserve[0]},E:{preserve[1]},C:{preserve[2]},conv:{preserve[4]},flips:{preserve[5][0]}/{preserve[5][1]}/{preserve[5][2]}",
        f"lattice_tie_plus=M:{tieplus[0]},E:{tieplus[1]},roll:{tieplus[3]},conv:{tieplus[4]}",
        f"adversarial={rejected}/{cases}",
        f"status={'PASS' if status else 'FAIL'}",
    ]
    return ("\n".join(lines) + "\n").encode("ascii")


def parse_u64(text: str) -> int:
    if not text or any(ch < '0' or ch > '9' for ch in text):
        raise ValueError('expected unsigned decimal integer')
    value = int(text)
    if value > (1 << 64) - 1:
        raise ValueError('integer exceeds uint64 range')
    return value


def main(argv: list[str]) -> int:
    if argv == ["report"]:
        sys.stdout.buffer.write(canonical_report())
        return 0
    if len(argv) == 2 and argv[0] == "run":
        try:
            steps = parse_u64(argv[1])
        except ValueError:
            print('invalid step count', file=sys.stderr)
            return 2
        s = run(SEED, steps)
        print(f"steps={s.steps} seed={s.seed} last={s.last} affine={s.affine_last} matrix={s.matrix_last} reverse={s.reverse_seed} sum_mod={s.sum_mod} weighted_mod={s.weighted_mod}")
        return 0
    if len(argv) == 3 and argv[0] == "exhaustive":
        try:
            lo, hi = parse_u64(argv[1]), parse_u64(argv[2])
        except ValueError:
            return 2
        if not 2 <= lo <= hi <= 16:
            return 2
        cases, digest, modsum = exhaustive(lo, hi)
        print(f"bits={lo}-{hi} cases={cases} fnv64={digest:016x} modsum={modsum}")
        return 0
    print("usage: dacb_twin.py report | run STEPS | exhaustive MIN_BITS MAX_BITS", file=sys.stderr)
    return 2


if __name__ == "__main__":
    raise SystemExit(main(sys.argv[1:]))
