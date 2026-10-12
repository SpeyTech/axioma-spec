# SRS-004-AMD-002 (`AX:OBS:v2`) conformance vectors (draft v0.2)

`vectors.txt` follows `VECTOR-FORMAT.md` v0.2 (test harness, not normative). It contains 51 lines:
- 21 OBS lines, for admissible records, including the exact 65,536-byte record;
- 30 REJECT lines, one per check, plus check-order cases.

The file was produced independently by two **new** clean-room implementations, each written only from the v0.2 amendment text, SRS-004-AMD-001 and this harness. Neither implementer saw the v0.1 implementations.

| Implementation | Language | SHA-256 | JSON encoder |
|---|---|---|---|
| `reference-python/obs_v2.py` | Python 3, stdlib (`hashlib`) | stdlib | own, from the SHALL-008 table (no `json` module) |
| `reference-c/obs_v2.c` | C99, no dependencies | own FIPS 180-4, self-tested at start-up | own |

**Results:**
- Both outputs are byte-identical: SHA-256 `08351eef082e10f7c8d1f10134a0937824bec76ef34ba263a77db6e588a5d815`.
- Both independently computed X = 1147 for the exact-limit case: 65,536 bytes is admitted, and 65,537 is rejected with `record_size`.
- **v0.1 continuity:** all 39 lines of the v0.1 set (`vectors-v0.1.txt`, `92c923e6…`) appear unchanged, and in order, in the v0.2 set. The v0.1 implementations in `reference-*/v0.1/` still reproduce `vectors-v0.1.txt`.
- Every computed rejection reason matched the text's expected reason in both implementations.
- The C implementer also rebuilt the base record by hand with `printf` and the system `sha256sum`, and got the same values.

Each implementer's ambiguity notes are kept: `IMPLEMENTER-NOTES-v0.2.md` beside the current implementations, and `v0.1/IMPLEMENTER-NOTES-v0.1.md` beside the earlier ones. The editorial clarifications in the draft answer them without changing any vector.

**Not covered here:**
- NFC (§8): see `nfc/`, Unicode 16.0.0 reference.

To reproduce:

```
python3 -I reference-python/obs_v2.py | cmp - vectors.txt
gcc -std=c99 -Wall -Wextra -Werror -pedantic -O2 reference-c/obs_v2.c -o obs2 && ./obs2 | cmp - vectors.txt
```

These are reference implementations for conformance only. No production `AX:OBS:v2` implementation exists or is authorised. `AX:OBS:v1` golden vectors and verification are unchanged.
