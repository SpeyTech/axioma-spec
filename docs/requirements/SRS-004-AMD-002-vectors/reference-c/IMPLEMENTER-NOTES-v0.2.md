# Implementer notes: AX:OBS:v2 clean-room C implementation (obs_v2.c)

Sources read: AMENDMENT.md (SRS-004-AMD-002 v0.2 DRAFT), RELATED-SRS-004-AMD-001.md (v1.0) and VECTOR-FORMAT.md. Nothing else was consulted.

Result: vectors-c.txt, 51 lines. Every computed rejection reason matches the "Expected reason" column of VECTOR-FORMAT.md Section 2. The exact-limit X was computed by the program as 1147 (stored length is 64389 + X; output_size is 33147, five digits).

## Ambiguities, contradictions and underspecified points

1. NFC checks not implemented (SHALL-015, SHALL-016, SHALL-020 steps 8 and 9). No UCD 16.0.0 data was available and the clean-room rules forbid fetching it. VECTOR-FORMAT.md step 1 permits skipping steps 8 and 9 because every harness output is NFC. Interpretation: the program skips them. This implementation is therefore not a complete v2 admission implementation and must not be presented as one; the NFC vectors of SHALL-017 (the `nfc/` directory) were not available either.

2. Producer-side transformations versus record-level validation (SHALL-021, SHALL-020, SHALL-013). SHALL-021 says a producer converts CRLF and lone CR to LF and truncates above 64,512 bytes before building a record. The harness supplies record fields directly. Interpretation: the harness exercises record-level admission only; no line-ending conversion or truncation is applied, so `cr_in_output` is rejected with `output_encoding` (as SHALL-021's last paragraph says), and a COMPLETE output longer than 64,512 bytes would not by itself be rejected because no SHALL-020 check covers it. The text does not say whether admission should reject a COMPLETE record whose output exceeds AX_MAX_OUTPUT_BYTES.

3. TRUNCATED consistency is weaker than SHALL-040 implies (SHALL-019, SHALL-021). SHALL-021 says a truncated output is cut at 64,512 bytes, yet SHALL-019 only requires output_size to be strictly greater than the output length. The harness `truncated` case (output "abc", output_size 100000) is admitted. Interpretation: only the SHALL-019 rule is enforced; there is no check that a TRUNCATED output is near 64,512 bytes.

4. Conditions of SHALL-022 checked at step 2 (member_not_permitted) use values that are range-checked only at step 3. Interpretation: the step 2 test compares output bytes to empty and output_size to 0 directly (raw values, before range checks). A record with rejected_record_size on a COMPLETE record and an out-of-range ledger_seq therefore gives member_not_permitted, matching the harness case `order_member_before_range`.

5. SHALL-022 "Every member other than those in SHALL-004 is likewise member_not_permitted", while SHALL-004 itself includes `rejected_record_size` and `obs_hash`. The text does not say how a missing required member, a duplicate member, a wrongly typed member (for example a string where an integer is expected) or a member out of order is reported; none of these has a reason in §9. Interpretation: out of scope for this harness; the in-memory model cannot express them.

6. Out-of-enum completion_state or failure_type is reported as `state_pair` (§9 row "or out of enum"). Applied to `unknown_state` (PARTIAL).

7. rejected_record_size range (SHALL-004 vs SHALL-006). SHALL-006 says every integer is within plus or minus (2^53 - 1), and SHALL-004 sets a lower bound of 65,537 for rejected_record_size. Interpretation: a value below 65,537 is `integer_range` (consistent with §9 "integer outside its range"), as in `rejected_size_too_small`. The text does not require the value to be consistent with any actual measurement, and there is no way to check it.

8. "valid UTF-8 in the strict sense of SHALL-012" (SHALL-011). SHALL-012 bundles UTF-8 validity, control exclusion and NFC. Interpretation: only the first bullet (well-formed, shortest form, no surrogates, at most U+10FFFF) is imported; identifiers are not NFC-checked, which §8 also says ("identifiers are governed by SHALL-011").

9. The identifier length limit is 256 bytes, not code points (SHALL-011). Applied as UTF-8 byte length of the raw value, before escaping.

10. Control ranges are tested on decoded code points, so C1 controls are detected as the two-byte sequences C2 80 to C2 9F. A raw byte 0x85 on its own is invalid UTF-8 and would also be rejected (by the same step for identifiers, by output_encoding for output).

11. Escape table versus admitted characters (SHALL-008). The table defines escapes for TAB, BS, FF, CR and other controls that admission never lets through. The encoder implements the full table anyway. U+007F is listed as written unescaped although admission rejects it in both identifiers and output. U+2028 and U+2029 are written raw (`del_free_unicode`).

12. obs_hash framing (SHALL-010). "LE64(len(P))" is read as the byte length of P, as an unsigned 64-bit little-endian integer. The tag is the 9 ASCII bytes `AX:OBS:v2` with no length prefix of its own.

13. record_size versus SRS-004-AMD-001 §2 (d) item 2 ("produce no obs_hash"). SHALL-020 step 10 computes obs_hash in order to measure the stored record. Interpretation: obs_hash is computed internally but discarded and never emitted on a size rejection, which satisfies both texts. The harness REJECT line carries no hash in any case.

14. What is measured for record_size. AMD-001 §2 (a) defines this for v1. Interpretation for v2: the octet count of the stored canonical record with obs_hash present, no terminator or framing; limit inclusive (65,536 admitted, 65,537 rejected).

15. Exact-limit case (VECTOR-FORMAT Section 1 case 21). Computed by building the record for X = 0 and X = 1, deriving the slope (1 byte per `a`), solving for X, checking that output_size keeps five digits, then rebuilding at X and confirming 65,536 bytes. X = 1147; X + 1 gives 65,537 and `record_size`.

16. The "input" check (SHALL-014) concerns a value that is not a record member. Interpretation: input is a separate admission argument (pointer and claimed length), used only to compute input_hash. An absent input with length 0 is treated as the empty input and hashes to e3b0...b855.

17. Verification-only reasons outside the admission order. `unknown_schema` and `schema_tag_mismatch` (SHALL-002) are excluded from the SHALL-020 order, so SHALL-020's claim that "every malformed input has exactly one reason" does not cover them, and their precedence relative to each other and to the admission checks during verification is not stated. SHALL-025 introduces `v1_after_cutover` and `v2_before_cutover`, which are missing from the §9 reason table. Not exercised by the harness.

18. Integer encoding of the Q16.16 fields (SHALL-004). temperature and top_p carry raw signed Q16.16 values as integers bounded by plus or minus (2^53 - 1), far beyond the 32-bit Q16.16 range. Interpretation: only the stated bound is checked. Similarly, seed has no narrower width given, unlike max_tokens.

19. Document control. The change record (§13) lists a "0.1 DRAFT (editorial)" row after the 0.2 row and says those clarifications changed no vector, yet SHALL-021 (cited in that row) is part of the v0.2 text. The ordering and versioning of that row are unclear. Also, the header says "No implementation is authorised" while §11 requires two independent implementations to produce the vectors; this implementation is treated as a conformance-vector exercise only.

20. AMD-001 §6 is informative and gives "about 32,000" LF characters as the v2 admissible count. With the BASE record the harness admits 32,000 escaped quotes (64,389 bytes); the exact LF maximum was not part of this harness and was not computed.

21. Member order claim (SHALL-004). The stated order was checked by hand against UTF-16 code unit ordering (all names ASCII) and is consistent, including `obs_hash` before `oracle_id`, `output` before `output_size`, and `rejected_record_size` between `params` and `schema_version`.
