# Mathematical specification

## 1. State transition

Let

```text
m = 2^32
a = 1664525
c = 1013904223
x0 = 1
```

and define

```text
T(x) = a*x + c (mod m).
```

The sequential benchmark computes `x_n = T^n(x_0)`.

Because the implementation uses `uint32_t`, conversion from the full unsigned product/sum to `uint32_t` implements reduction modulo `2^32`. Intermediate multiplication is performed in `uint64_t`, so the C source does not depend on signed overflow.

## 2. Affine-transform oracle

Represent an affine transform as `(p,q)`:

```text
F(x) = p*x + q (mod m).
```

Composition is

```text
(p1,q1) o (p2,q2)
= (p1*p2, p1*q2 + q1) (mod m).
```

Binary exponentiation computes `T^n = (p_n,q_n)` in `O(log n)` transform compositions. The oracle state is then

```text
p_n*x0 + q_n (mod m).
```

This algorithm does not enumerate the `n` sequential states.

## 3. Matrix oracle

The same transition can be represented independently as

```text
| a c | | x |   | ax+c |
| 0 1 | | 1 | = |  1   |  (mod m).
```

The matrix oracle computes

```text
| a c |^n
| 0 1 |
```

by binary exponentiation and applies it to `(x0,1)`.

The affine and matrix forms are mathematically equivalent representations, but the implementations use different data structures and multiplication routines. DACB compares both against the sequential chain.

## 4. Reverse oracle

`a` is odd, so it has a multiplicative inverse modulo `2^32`.

DACB computes the inverse with Newton/Hensel refinement in unsigned arithmetic and verifies

```text
a * a_inverse = 1 (mod 2^32).
```

The inverse transition is

```text
T^-1(x) = a_inverse * (x-c) (mod 2^32).
```

After a forward run of `n` steps, applying the inverse exactly `n` times must recover the original seed.

For the canonical multiplier:

```text
a_inverse = 4276115653
```

## 5. Sequence fingerprints

For states `x1 ... xn`, DACB records two independent aggregate fingerprints modulo the prime `1,000,000,007`:

```text
sum_mod      = sum(x_i) mod p
weighted_mod = sum(i*x_i) mod p, i=1..n
```

The checksum is not used as a cryptographic hash. It is a compact deterministic regression fingerprint in addition to the exact final-state oracles.

Any reported remainder must satisfy

```text
0 <= remainder < 1,000,000,007.
```

This gives an immediate rejection rule for impossible candidate outputs.

## 6. Full-period condition

For modulus `2^k`, a mixed affine congruential recurrence has full period when the increment is odd and the multiplier is `1 mod 4`.

Here:

```text
1013904223 is odd
1664525 mod 4 = 1
```

DACB nevertheless does not rely only on the theorem for its reduced domains. For powers `2^2 .. 2^12`, the C verifier directly walks every seed through every step up to the complete period and checks early-return/period behavior.

The full `2^32` state space is not exhaustively enumerated in the release campaign.

## 7. Exact finite-number embedding

All canonical states are non-negative integers below `2^32`. The test suite embeds each of the first 5,000 states into exactly equivalent finite dyadic forms:

```text
x/1 = 2x/2 = 4x/4.
```

Equality is checked using integer arithmetic only. No floating-point approximation is used for this test.
