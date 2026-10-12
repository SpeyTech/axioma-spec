# SRS-007-AMD-001 conformance vectors (draft)

`vectors.txt` follows `VECTOR-FORMAT.md` (v0.3), the test harness definition, which is not normative. It was produced independently by two clean-room implementations written from the amendment's **v0.3** text. Each implementer saw only `SRS-007-AMD-001-v0.3-DRAFT.md` and `VECTOR-FORMAT.md`: no estate Merkle or SHA-256 code, no earlier vectors, and not the other's work.

| Implementation | Language | SHA-256 | Output |
|---|---|---|---|
| `reference-python/merkle_v2.py` | Python 3, stdlib only (`hashlib`) | stdlib | 74 lines |
| `reference-c/merkle_v2.c` | C99, no dependencies | own FIPS 180-4 code, self-tested at start-up | 74 lines |

Both produce byte-identical output, SHA-256 `2d3e53b8f48d793a7d88c2b8401ed8cb1f2720ceb865dd2bc4843724351e705f`. The output has four kinds of line:

| Lines | Kind | What they hold |
|---|---|---|
| 10 | ROOT | closure roots |
| 43 | PROOF | inclusion proofs; all VERIFIED |
| 15 | REJECT | rejection cases |
| 6 | V1ROOT | Appendix A v1 roots |

Further cross-checks:
- **v0.2 continuity:** the first 65 lines are byte-identical to `vectors-v0.2.txt` (`96e66cd1…0a4efd`), which the v0.1 implementations in `reference-*/v0.1/` still reproduce.
- **v1 fidelity:** the V1ROOT lines equal axioma-governance `ax_merkle_compute_root` at `93d75a6` for the same lists, so Appendix A restates the frozen v1 construction exactly.
- **Implementer notes:** each implementer's notes on ambiguities are kept as `IMPLEMENTER-NOTES-v0.3.md`. The editorial clarifications in v0.3 resolve them and change no vector.

To reproduce:

```
python3 -I reference-python/merkle_v2.py | cmp - vectors.txt
gcc -std=c99 -Wall -Wextra -Werror -pedantic -O2 reference-c/merkle_v2.c -o mv2 && ./mv2 | cmp - vectors.txt
gcc -std=c99 -fsanitize=address,undefined -g reference-c/merkle_v2.c -o mv2s && ./mv2s | cmp - vectors.txt
head -65 vectors.txt | cmp - vectors-v0.2.txt
```

These are reference implementations for conformance only. They are not the production implementation and are not linked by any library. Production code in axioma-governance and axioma-sdk must reproduce `vectors.txt` exactly before it is accepted (SHALL-013). No Merkle v2 implementation in axioma-governance or axioma-sdk is authorised while the amendment is in draft.
