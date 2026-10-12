# SRS-007-AMD-001 conformance vectors (draft)

`vectors.txt` follows `VECTOR-FORMAT.md`, the test harness definition, which is not normative. It was produced independently by two clean-room implementations written from the amendment's v0.1 text:

| Implementation | Language | SHA-256 | Output |
|---|---|---|---|
| `reference-python/merkle_v2.py` | Python 3, stdlib only (`hashlib`) | stdlib | 65 lines |
| `reference-c/merkle_v2.c` | C99, no dependencies | own FIPS 180-4 code, self-tested at start-up | 65 lines |

Both produce byte-identical output, SHA-256 `96e66cd14f39972fc79444498407f0f4fca37bb2de82dfad9e2840bd8f0a4efd`. Each implementer's notes on ambiguities in the v0.1 text are kept as `IMPLEMENTER-NOTES-v0.1.md`; amendment v0.2 resolves them.

To reproduce:

```
python3 -I reference-python/merkle_v2.py | cmp - vectors.txt
python3 reference-python/selftest.py
gcc -std=c99 -Wall -Wextra -Werror -pedantic -O2 reference-c/merkle_v2.c -o mv2 && ./mv2 | cmp - vectors.txt
gcc -std=c99 -fsanitize=address,undefined -g reference-c/merkle_v2.c -o mv2s && ./mv2s | cmp - vectors.txt
```

These are reference implementations for conformance only. They are not the production implementation and are not linked by any library. Production code in axioma-governance and axioma-sdk must reproduce `vectors.txt` exactly before it is accepted (SHALL-013). Nothing here is implemented in axioma-governance yet, pending owner review of the amendment.
