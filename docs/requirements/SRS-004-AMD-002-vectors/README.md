# SRS-004-AMD-002 (`AX:OBS:v2`) conformance vectors (draft)

`vectors.txt` follows `VECTOR-FORMAT.md` (test harness, not normative). It contains 39 lines:
- 17 OBS lines, for admissible records;
- 22 REJECT lines, one per check, plus check-order cases.

The file was produced independently by two clean-room implementations, each written only from the v0.1 amendment text and this harness.

| Implementation | Language | SHA-256 | JSON encoder |
|---|---|---|---|
| `reference-python/obs_v2.py` | Python 3, stdlib (`hashlib`) | stdlib | own, from the SHALL-008 table (no `json` module) |
| `reference-c/obs_v2.c` | C99, no dependencies | own FIPS 180-4, self-tested at start-up | own |

**Results:**
- Both outputs are byte-identical: SHA-256 `92c923e65d98657c2129dfb17a27fb2984270828a031e1604fca5af804dea4b7`.
- Every computed rejection reason matched the text's expected reason in both implementations.
- The C implementer also rebuilt the base record by hand with `printf` and the system `sha256sum`, and got the same values.

Each implementer's ambiguity notes are kept as `IMPLEMENTER-NOTES-v0.1.md`. The editorial clarifications in the draft answer them without changing any vector.

**Not covered here:**
- NFC (§8): see `nfc/`, Unicode 16.0.0 reference.
- The exact 65,536-byte record boundary: SRS-004-AMD-001 already tests it for v1; a v2 boundary vector is to be added with the next revision.

To reproduce:

```
python3 -I reference-python/obs_v2.py | cmp - vectors.txt
gcc -std=c99 -Wall -Wextra -Werror -pedantic -O2 reference-c/obs_v2.c -o obs2 && ./obs2 | cmp - vectors.txt
```

These are reference implementations for conformance only. No production `AX:OBS:v2` implementation exists or is authorised. `AX:OBS:v1` golden vectors and verification are unchanged.
