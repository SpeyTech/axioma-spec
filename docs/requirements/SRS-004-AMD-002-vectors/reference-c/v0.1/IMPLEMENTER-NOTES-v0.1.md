# Implementer notes: AX:OBS:v2 clean-room C implementation

Sources read: AMENDMENT.md (SRS-004-AMD-002 v0.1 DRAFT) and VECTOR-FORMAT.md only. Each point below names the section, the problem, and the interpretation used in obs_v2.c.

## Ambiguities, contradictions and underspecified points

1. **Record size threshold (SHALL-013, SHALL-020 step 9, §9 `record_size`).** The text says "over 65,536 bytes" and refers to SRS-004-AMD-001, which is not supplied. I reject only when the stored record length is strictly greater than 65,536 bytes, so exactly 65,536 is admitted. No vector sits at the boundary (quote_heavy_fits is 64,389 bytes, quote_heavy_over is about 66,389), so the vectors do not test this choice.

2. **Record size needs the hash first (SHALL-020 step 9).** Step 9 is "computed on the stored record", which contains `obs_hash`. Admission therefore has to compute `input_hash` and `obs_hash` before it can do the last check. I do that. Since `obs_hash` is always 64 hex characters, the length is known in advance, but the text does not say so.

3. **"Applied after its normalisation" (SHALL-012) against "SHALL NOT normalise" (SHALL-016).** SHALL-012 says the control-character rule is SHALL-043 "carried over, applied after its normalisation". SHALL-043 is not supplied, and SHALL-016 forbids normalising, repairing or substituting text. I apply the control check to the raw output bytes with no normalisation of any kind.

4. **AX_MAX_OUTPUT_BYTES (SHALL-013).** Truncation under SRS-004-SHALL-040 is said to apply, but the limit value is not given. The harness supplies TRUNCATED records directly, so no truncation is performed. Nothing in the text relates `output_size` for TRUNCATED records to AX_MAX_OUTPUT_BYTES beyond "greater than or equal".

5. **NFC steps skipped (SHALL-016, SHALL-017; VECTOR-FORMAT step 1).** I did not implement NFC or the 30-non-starter bound; the harness permits skipping steps 7 and 8. A full implementation needs UCD 16.0.0 data, which was out of scope here. Also, §8 talks of "a string s", but §9 attaches `not_nfc` and `nfc_bound` to output only. My reading is that identifiers are not NFC-checked.

6. **Identifier UTF-8 strictness (SHALL-011).** SHALL-011 says only "valid UTF-8", while SHALL-012 spells out well-formed, shortest form, no surrogates and at most U+10FFFF. I use the same strict validator for both.

7. **Identifier failure kinds share one reason (SHALL-011).** Empty, oversized, invalid UTF-8 and forbidden-control identifiers all give `identifier`. For an identifier I check length (non-empty, at most 256 bytes) before encoding and controls. The order does not change the reason, but it does decide which sub-rule fires first.

8. **Output encoding vs control (SHALL-012, §9).** Invalid UTF-8 and forbidden controls both map to `output_encoding`. I decode left to right and fail at the first offending code point or byte sequence.

9. **NULL input with zero length (SHALL-014).** The rule rejects only `NULL` with a non-zero length, and says "the empty input is admissible". `NULL` with length 0 is not covered explicitly. I treat it as the empty input and hash it as zero bytes. No vector exercises this.

10. **Commitment tag location (SHALL-001, SHALL-002).** The commitment tag is described as separate from `schema_version`, but the record has no member that carries it, and the text does not say where a verifier reads it. As a result `schema_tag_mismatch` cannot be computed from the record alone. SHALL-020 also leaves `unknown_schema` and `schema_tag_mismatch` out of the admission order and says they "apply to verification of stored records", without saying where they fall relative to the other checks during verification. The harness never sets `schema_version`, so neither reason is produced. The producer always writes `"AX:OBS:v2"`.

11. **"Out of enum" (SHALL-005, §9 `state_pair`).** An unknown `completion_state` (PARTIAL) or unknown `failure_type` is reported as `state_pair` and not as a separate enum error, as §9 says. I compare enum values byte for byte, case-sensitive.

12. **Integer range check order within step 2 (SHALL-020).** The order is ledger_seq, output_size, then params in member order. All three report the same reason, so the order cannot be observed. Negative `ledger_seq` or `output_size` is `integer_range` (SHALL-006), not `output_size`.

13. **TRUNCATED with out-of-range output_size (VECTOR-FORMAT §2 case 4).** The case sets output_size to 2^53 with output "ok". This fails step 2. It would pass step 6, because 2^53 is not smaller than 2. Step 2 is reported, which is consistent with the text.

14. **Default output_size (VECTOR-FORMAT BASE).** "Byte length of output, unless a case sets it" is applied after the case's output change. For invalid_utf8_output that is 3 (raw bytes). In my harness code the "not set" state is an internal sentinel of -1; no case sets -1, so this cannot clash.

15. **Python-style escapes vs literal characters (VECTOR-FORMAT Notation, cases 5, 14, 17).** The Notation section lists `\uXXXX` and `\U0001XXXX` escapes, but cases 5, 14 and 17 also contain literal non-ASCII characters in the Markdown. I read the raw bytes of those lines: é is precomposed U+00E9 (C3 A9), Å is precomposed U+00C5 (C3 85), ω is U+03C9, è is U+00E8. Case 17 is exactly U+00A0 U+2028 U+2029 with no other characters between the quotes. If a copy of the file were re-saved in NFD, cases 5 and 14 would change meaning, and case 5 would stop being NFC. That breaks the statement in step 1 that every harness output is NFC.

16. **Output file layout (VECTOR-FORMAT "Output lines").** The text does not say whether section headers or blank lines appear between sections. I print one line per case, Section 1 then Section 2, with no headers and a final LF after the last line. `len` is decimal.

17. **Member order wording (SHALL-004).** The table lists the members of the stored record. The hashed form drops `obs_hash` from between `model_id` and `oracle_id`. That is explicit in SHALL-010 but marked only as "omitted in the hashed form" in SHALL-004. Both are consistent.

18. **LE64 length (SHALL-010).** `len(P)` is taken as the byte length of the UTF-8 canonical form, not a count of code points or characters. The text implies this but does not state it.

19. **Escape table vs admitted content (SHALL-008).** The table covers U+007F and non-ASCII "unescaped", and U+2028/U+2029 are not escaped (RFC 8785 follows ECMAScript JSON.stringify, which since ES2019 does not escape them). My encoder implements the whole table, including `\b`, `\t`, `\f`, `\r` and `\u00xx`, even though admission means only `\"`, `\\` and `\n` can occur.

20. **Clause numbering (§7).** SHALL-019 appears between SHALL-012 and SHALL-013, and SHALL-018 sits in §11. This is editorial only, but it can confuse anyone following the clauses in order.

21. **params ranges (SHALL-004, SHALL-006).** `max_tokens` is allowed to be negative (±(2^53 − 1)), which is semantically odd, but I follow the text. `temperature` and `top_p` are raw Q16.16 integers with no tighter bound.

## Verification performed

- SHA-256 self-test at start-up on the three FIPS vectors (the program exits 1 on failure).
- The base case was rebuilt by hand with shell `printf` and the system `sha256sum`. It gave the same len (387), obs_hash and record_sha256 as the program.
- Every computed reject reason matched the expected column.
