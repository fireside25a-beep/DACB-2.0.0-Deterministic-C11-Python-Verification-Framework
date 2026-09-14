#!/usr/bin/env python3
import importlib.util
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
SPEC = importlib.util.spec_from_file_location("dacb_twin", ROOT / "python_twin" / "dacb_twin.py")
mod = importlib.util.module_from_spec(SPEC)
assert SPEC and SPEC.loader
sys.modules[SPEC.name] = mod
SPEC.loader.exec_module(mod)


def main() -> int:
    vectors = {
        1: (1015568748, 15568741, 15568741),
        2: (1586005467, 601574201, 187579654),
        3: (2165703038, 767277225, 684688726),
        10: (2745540835, 131652221, 898836369),
        100: (2146152485, 492427211, 623809866),
        500: (2563561269, 523200049, 256699986),
        1000: (645503657, 63521775, 299065930),
        5000: (4125282121, 585303980, 554861868),
        100000: (694140833, 784266586, 455600981),
        1000000: (366300225, 346639628, 152047219),
    }
    for steps, expected in vectors.items():
        s = mod.run(mod.SEED, steps)
        assert (s.last, s.sum_mod, s.weighted_mod) == expected
        assert s.last == s.affine_last == s.matrix_last
        assert s.reverse_seed == mod.SEED

    assert mod.inverse_multiplier32() == 4276115653
    for x in (0, 1, 0xFFFFFFFF, 1015568748, 4125282121):
        assert mod.prev32(mod.step32(x)) == x

    for n in (0, 1, 2, 3, 10, 100, 500, 1000, 5000, 65537):
        assert mod.affine_state(1, n) == mod.matrix_state(1, n)

    cases, digest, modsum = mod.exhaustive(2, 10)
    assert cases == 1_400_140
    assert digest == 0x9F80ADE0CA124045
    assert modsum == 220_519_645

    preserve = mod.lattice(False)
    plus = mod.lattice(True)
    assert preserve[0] == -1816 and preserve[1] == -4296 and preserve[2] == 13_134_816
    assert preserve[4] == 3 and preserve[5] == (987, 38, 0)
    assert plus[0] == 4096 and plus[1] == -8192 and plus[3] == 299_341_384 and plus[4] == 5

    truth = mod.run(1, 5000)

    # Mutation rejection: wrong increment, wrong multiplier, and checksum indexing.
    xi = xm = canonical = 1
    off_by_one = 0
    for i in range(5000):
        xi = (mod.A * xi + mod.C + 2) & mod.MASK32
        xm = ((mod.A + 2) * xm + mod.C) & mod.MASK32
        canonical = mod.step32(canonical)
        off_by_one = (off_by_one + i * (canonical % mod.CHECK)) % mod.CHECK
    assert xi != truth.last
    assert xm != truth.last
    assert off_by_one != truth.weighted_mod
    assert mod.A * 0xFFFFFFFF + mod.C > mod.MASK32
    assert ((mod.A * 0xFFFFFFFF + mod.C) & mod.MASK32) == mod.step32(0xFFFFFFFF)

    bad = (3_845_503_477, 4_566_409_260, 1_247_804_322, True, True, 2)
    issues = mod.audit(truth, bad)
    assert issues & mod.SUM_OUT_OF_RANGE
    assert issues & mod.WEIGHT_OUT_OF_RANGE
    assert issues & mod.LAST_MISMATCH
    state, fixed, correction_issues = mod.self_correct(5000, bad)
    assert state == "CORRECTED" and correction_issues != 0
    assert fixed[:3] == (truth.last, truth.sum_mod, truth.weighted_mod)
    stale = (truth.last, truth.sum_mod, truth.weighted_mod, False, True, 3)
    state, _, correction_issues = mod.self_correct(5000, stale)
    assert state == "REFUSE" and correction_issues & mod.SOURCE_MISMATCH

    # Exact finite dyadic duplicate forms: x/1 == 2x/2 == 4x/4.
    x = 1
    for _ in range(5000):
        x = mod.step32(x)
        assert x * 2 == (x << 1)
        assert x * 4 == (x << 2)
        assert (x << 1) * 2 == (x << 2)

    report = mod.canonical_report()
    assert report.endswith(b"status=PASS\n")
    print("DACB_PYTHON_TWIN_TESTS_PASS")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
