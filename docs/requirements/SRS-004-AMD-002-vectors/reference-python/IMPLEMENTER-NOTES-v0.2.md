# Implementer notes: AX:OBS:v2 (Python, clean room)

Sources read: AMENDMENT.md (SRS-004-AMD-002 v0.2 DRAFT), RELATED-SRS-004-AMD-001.md, VECTOR-FORMAT.md. Nothing else.

Result: every computed reason matches the expected column of VECTOR-FORMAT.md Section 2, and every Section 1 case is admitted. The exact-limit value computed by the program is X = 1147 (stored record 65,536 bytes; X + 1 gives 65,537 and is rejected with record_size).

## Ambiguities, contradictions and underspecified points

1. Measured quantity for record_size (SHALL-020 step 10, SHALL-013). The amendment says "stored record" and points at SRS-004-AMD-001, whose §2(a) defines the measured quantity only for AX:OBS:v1 (SHALL-006 plus SHALL-045 escaping). I applied the same definition to v2: octets of the v2 canonical form with obs_hash present, excluding terminator, framing, tag and length prefix.

2. Type errors have no reason code (§9, SHALL-006). Nothing says what to report when, for example, ledger_seq is a string, null, or a non-integer number, or when params is not an object. I report integer_range for a null or non-integer ledger_seq, output_size or rejected_record_size, and for non-integer params values. The harness does not exercise this.

3. Range of rejected_record_size reported as integer_range (SHALL-004 vs SHALL-006). SHALL-006 defines integer_range as "outside ±(2^53 − 1)" plus non-negativity for two members, while SHALL-004 sets narrower ranges for max_tokens (0 to 2,147,483,647) and rejected_record_size (65,537 upward). §9 says integer_range covers "integer outside its range", which I read as the per-member range of SHALL-004. That gives integer_range for rejected_size_too_small and max_tokens_negative/over, matching the expected column.

4. member_not_permitted conditions are evaluated before output is validated (SHALL-020 steps 2 and 6, SHALL-022). SHALL-022 requires `output = ""` and `output_size = 0`. At step 2 the output has not yet been checked for encoding, and output_size not for range. I compare raw output bytes for zero length and require output_size to be the integer 0; any other value (including an out-of-range one) gives member_not_permitted at step 2. Case 30 (order_member_before_range) confirms member_not_permitted precedes integer_range.

5. Unknown members and the "input" field. SHALL-004 lists record members; input itself is not a member (only input_hash is), and input_hash and obs_hash are computed, not supplied. The admission interface is therefore not defined. I modelled the admission input as the record fields plus input bytes and a claimed input length. A caller-supplied input_hash, obs_hash or schema_version is not modelled. Also unspecified: whether an unknown member inside params is member_not_permitted (I chose yes).

6. NULL input with zero length (SHALL-014). "input == NULL with a non-zero length SHALL be rejected" is a C interface rule in a record format; NULL with length 0 is not stated. I treat it as the empty input (hash e3b0...b855). Not exercised.

7. TRUNCATED consistency with AX_MAX_OUTPUT_BYTES (SHALL-021, SHALL-040, SHALL-019). The producer truncates at 64,512 bytes, but admission only requires output_size > len(output). The harness truncated case (output "abc", output_size 100000) is admitted though no real producer could make it. I did not add a check that a TRUNCATED output is near 64,512 bytes or that a COMPLETE output is at most 64,512 bytes, since no rule or reason code requires it. Likewise the exact-limit and quote-heavy outputs are under 64,512 bytes, so the question never arises in the harness.

8. Truncation boundary wording (SHALL-021 step 2). "Cut at that length, at a UTF-8 character boundary" does not say whether the cut is at the last boundary at or below 64,512. I assume at or below. Not exercised (producer side only).

9. NFC steps 8 and 9 not implemented (§8, VECTOR-FORMAT step 1). The task forbids unicodedata and the harness permits skipping. The implementation is therefore not conformant to SHALL-016/SHALL-017 in general; it only produces this vector set. Also, §8 says NFC "applies to output" while SHALL-011 does not require NFC for identifiers; I followed that literally.

10. Identifier length unit (SHALL-011). "At most 256 bytes" is taken as UTF-8 byte length. "Valid UTF-8 in the strict sense of SHALL-012" refers to the first bullet only (well-formed, shortest form, no surrogates, at most U+10FFFF); I did not apply SHALL-012's NFC bullet to identifiers.

11. Identifier sub-check order (SHALL-020 step 4). The reason is the same for every identifier failure, so the order of the four SHALL-011 tests within one identifier does not matter; model_id is checked before oracle_id as stated.

12. Escaping table versus admission (SHALL-008). The encoder implements the full table (\b, \t, \n, \f, \r, \u00xx lowercase) even though admission makes all but \", \\ and \n unreachable. U+007F, U+2028 and U+2029 are written raw, matching RFC 8785 and case del_free_unicode.

13. Member order statement (SHALL-004). The table lists rejected_record_size between params and schema_version, consistent with code-unit order. obs_hash sits between model_id and oracle_id in the stored record and is removed with its preceding comma context ("no name, no value, no comma"). Since obs_hash is never first or last, removing it means removing `"obs_hash":"<hex>",`. The code asserts this property.

14. Dispatch checks absent from admission (SHALL-002, SHALL-020 note). unknown_schema and schema_tag_mismatch apply only to verification of stored records and need a ledger frame tag that the harness does not provide. Admission always writes schema_version AX:OBS:v2 and these reasons are never produced here. SHALL-025's v1_after_cutover and v2_before_cutover reasons are also not in the §9 table, which claims to list all reasons.

15. obs_hash input framing (SHALL-010). LE64 is taken as unsigned 64-bit little-endian of len(P) in bytes. The tag is the 9 ASCII bytes "AX:OBS:v2" with no terminator, as stated.

16. Integer formatting (SHALL-006). Implemented as plain decimal; no floats or exponent form arise because every numeric member is an integer. temperature and top_p are raw Q16.16 integers; the format never sees fractional values.

17. "max_tokens ... (the v1 field width)". The parenthetical suggests a signed 32-bit field, but the range is stated explicitly so the width is not needed.

18. Exact-limit definition (VECTOR-FORMAT Section 1 case 21). The stored length is linear in X with slope 1 while output_size keeps five digits. The program computes L(0) and L(1), checks the slope is 1, solves X = 65,536 − L(0), verifies L(X) = 65,536 and that output_size 32000 + X still has five digits. Result X = 1147, output_size 33147.

19. SRS-004-AMD-001 §6 says the v2 admissible LF count is "about 32,000". This is informative only and is consistent with the computed sizes (quote_heavy_fits, 32,000 two-byte escapes, gives 64,389 bytes).

20. Change record inconsistencies (§13). The "0.1 DRAFT (editorial)" row appears after the 0.2 row although both are dated 2026-10-12; and it says the clarifications changed no vector, yet it introduces the strict "output_size > length" rule for TRUNCATED, which would change the result for a TRUNCATED record with equality (v1 accepted it). No harness case uses equality, so the claim may hold for the vector set only. Also the header says "Vectors regenerated by two new independent implementations" while SHALL-018 and the status line say no implementation is authorised; I treated this exercise as conformance-vector work only.

21. SHALL-012 forbids control characters "other than U+000A" while SHALL-021 converts CR on the producer side; a CR reaching admission is rejected as output_encoding (case cr_in_output), as SHALL-021 states. Admission does not itself convert CR.
