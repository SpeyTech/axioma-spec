# SRS-004-AMD-002 section 8: targeted NFC vectors (Unicode 16.0.0)

`nfc-targeted-u16.txt` holds 50 cases, one per line: `<name> <hex code points> <verdict>`.
- The verdict is `ACCEPT`, `REJECTED(not_nfc)` or `REJECTED(nfc_bound)`, defined by SHALL-016.
- The bound is checked first.

| Group | Cases |
|---|---|
| Indic scripts | Devanagari, Bengali, Gurmukhi, Gujarati, Oriya, Tamil, Telugu, Kannada, Malayalam: nukta, two-part vowel signs, length marks, whole words |
| Other scripts | Latin and compatibility singletons; Hangul jamo |
| Ordering | canonical ordering |
| Bound | 30, 31 and post-decomposition runs |
| New in 16.0 | one letter assigned in 16.0 (U+10D50), which must be accepted. It does not discriminate versions: unassigned code points are NFC-stable, so 15.1 also accepts it |

**Reference:** `nfc_reference.py`, run with the PyPI package `unicodedata2` at exactly 16.0.0. The script refuses any other version. That package is independent of all Axioma code. As a cross-check, Python's built-in `unicodedata` (Unicode 15.1) gives the same verdict for all 50 cases. Reproduce with:

```
python3 -m venv v && v/bin/pip install --only-binary :all: unicodedata2==16.0.0
v/bin/python -I nfc_reference.py | cmp - nfc-targeted-u16.txt
```

**Still required for SHALL-017:** the NFC column of the UCD 16.0.0 `NormalizationTest.txt`. That file could not be fetched in the preparation environment, because unicode.org is blocked by its network policy. Fetch it on a controlled host (`https://www.unicode.org/Public/16.0.0/ucd/NormalizationTest.txt`), record its SHA-256, and add it before any implementation is accepted.

No implementation exists or is authorised (owner direction 2026-10-12: Option B, v2 only, not before the versioned protocol decision).
