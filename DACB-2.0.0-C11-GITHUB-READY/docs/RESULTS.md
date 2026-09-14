# Verified release results

Version: `2.0.0`

## Canonical vectors

```text
steps=5000
last=4125282121
sum_mod=585303980
weighted_mod=554861868

steps=100000
last=694140833
sum_mod=784266586
weighted_mod=455600981

steps=1000000
last=366300225
sum_mod=346639628
weighted_mod=152047219
```

## Known checkpoints

```text
1       1015568748
2       1586005467
3       2165703038
10      2745540835
100     2146152485
500     2563561269
1000    645503657
5000    4125282121
100000  694140833
1000000 366300225
```

## Canonical reduced-domain report

```text
bits=2..10
cases=1,400,140
FNV64=9f80ade0ca124045
modsum=220519645
```

## Deep cross-language reduced-domain gate

```text
bits=2..11
cases=5,596,492
FNV64=bb145fc9fd64af85
modsum=701664029
```

## Extended C gate

```text
bits=2..12
cases=22,377,804
FNV64=d6fcc0f67a842585
modsum=341034241
```

## Canonical report digest

```text
SHA-256 = 1d2ffc1bd5703d8514a62226e42bc007798f454c9c935d974071d6add57d7679
```

The report bytes are identical for GCC C11, Clang C11, and the independent Python twin.

See `evidence/RELEASE_VERIFICATION.txt` for the sealed release gate transcript.
