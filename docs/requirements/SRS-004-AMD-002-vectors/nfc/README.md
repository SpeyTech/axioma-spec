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

**Full conformance run (SHALL-017):** `nt16_conformance.py NormalizationTest.txt`, run with the 16.0.0 reference installed. It does the following:
1. Refuses any file whose SHA-256 is not the owner-reported `d811971453e7075e1ad56fb1b301eece5aa80757b81f6156e74a1bfb3ae5ceb1`, or whose header does not name 16.0.0.
2. Checks the reference against all five UAX #15 NFC invariants on every line.
3. Checks that every code point outside Part 1 is NFC-invariant.
4. Derives the SHALL-016 verdict of every column string from the file's own columns, and requires the reference to agree everywhere.
5. Re-checks the targeted vectors.

The harness has been exercised on a synthetic file built from the reference: it passes, and it fails when one line is corrupted. That proves only the plumbing. **The conformance result exists only once the owner's file bytes have been transferred and the run has passed on them.**

**Status:** the owner obtained `NormalizationTest.txt` 16.0.0 on the owner-controlled host: 20,026 lines, header dated 2024-04-30, SHA-256 `d811971453e7075e1ad56fb1b301eece5aa80757b81f6156e74a1bfb3ae5ceb1`. The bytes were transferred by the owner through git: they are `ucd16/NormalizationTest.txt` (commit `b1b9059` on `unicode/nt16-2026-10`), and their digest was verified on receipt.

**Conformance result (2026-10-12): PASS** (`ucd16/CONFORMANCE-RESULT-2026-10-12.txt`):
- 19,965 test lines; the reference holds every UAX #15 NFC invariant on every line;
- every code point outside Part 1 is NFC-invariant;
- all 99,825 column strings: the admission verdict derived from the file's own columns equals the reference verdict, with no over-bound strings;
- the 50 targeted vectors agree;
- derived verdict list SHA-256 `da5aca59a5fffbf42abc01de9e9a6b2410a69f4723a8ce34298f55ff6db08345`.

**Reproducibility record** (`ucd16/CONFORMANCE-TRANSCRIPT-2026-10-12-rerun.txt`, added after `f688986`, which is unchanged): a rerun of the same harness on the same file, with its verbatim output, which is identical to the result above. It records:
- the interpreter (CPython 3.13.16);
- the reference: unicodedata2 16.0.0, `unidata_version` 16.0.0, from the PyPI wheel `unicodedata2-16.0.0-cp313-cp313-manylinux_2_17_x86_64.manylinux2014_x86_64.whl` with SHA-256 `64fc29fd…37a6`; the installed module is identical to the one in the wheel;
- the harness SHA-256 `c35ac9c8…609f`;
- the input SHA-256 and git blob;
- the exact command line and exit status;
- the original run's command, from the session record.

This confirms the reference and the vectors. It does not certify any implementation; none exists or is authorised.

No implementation exists or is authorised (owner direction 2026-10-12: Option B, v2 only, not before the versioned protocol decision).
