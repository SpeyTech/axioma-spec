# Implementer notes: AX:OBS:v2 Python clean-room implementation

Sources read: AMENDMENT.md (SRS-004-AMD-002 v0.1 DRAFT) and VECTOR-FORMAT.md only. Implementation: obs_v2.py (Python 3 standard library, hand-written canonical encoder, hashlib for SHA-256). Output: vectors-py.txt, 39 lines. Every computed rejection reason matched the "Expected reason" column; no discrepancies.

Each item below gives the location, the problem, and the interpretation used.

## A. AMENDMENT.md

1. SHALL-013 and AX_MAX_OUTPUT_BYTES. SHALL-013 imports SRS-004-SHALL-040 (truncation above AX_MAX_OUTPUT_BYTES) but the amendment never gives the value, and SHALL-020 has no check step for it. The vector case `truncated` uses output_size 100000. Interpretation: no truncation-limit check is performed at admission; only SHALL-019 (output_size >= byte length for TRUNCATED) applies.

2. SHALL-019, TRUNCATED with output_size equal to the output length. The text says "greater than or equal", so a TRUNCATED record whose output was not actually shortened is admissible. This looks semantically odd (nothing was truncated) but I followed the text literally: equality is accepted.

3. SHALL-019 placement and numbering. SHALL-019 sits between SHALL-012 and SHALL-013, and SHALL-020 follows SHALL-018 in section 9. Numbering is non-sequential. No behavioural effect.

4. SHALL-020 step 9 and SHALL-013 (record_size). "Over 65,536 bytes" is read as strictly greater than 65536; a stored record of exactly 65536 bytes is admitted. The limit is measured on the stored record (canonical form with obs_hash present), as step 9 says. This means obs_hash must be computed before the final admission check, so hashing is interleaved with admission rather than strictly after it.

5. SHALL-020 omits schema checks. unknown_schema and schema_tag_mismatch are said to apply only to verification of stored records. Admission sets schema_version to the constant "AX:OBS:v2". Interpretation: these two reasons are never produced by admission in this implementation.

6. SHALL-002 "commitment tag". The commitment tag is the prefix of the hashed preimage in SHALL-010; it is not a stored member of the record. It is unclear how a verifier observes a tag that "disagrees" with schema_version from the stored record alone (perhaps from the ledger or chain context). Not exercised; not implemented.

7. Verifier procedure underspecified (SHALL-002, SHALL-010). The amendment says a verifier recomputes P by removing the obs_hash member, but does not say how the stored bytes are parsed, whether duplicate or extra members, non-canonical whitespace, or non-canonical number forms in a stored record are rejected and with which reason, or whether the verifier re-runs all admission checks. Only admission is implemented. As a self-check the implementation asserts that deleting the exact bytes `"obs_hash":"<hex>",` from the stored record yields P byte for byte.

8. SHALL-010 "no name, no value, no comma". Because obs_hash is never the last member, removing it removes one adjacent comma. Which comma (before or after) is not stated, but the result is identical either way. Interpretation: P is produced by encoding the record without that member.

9. SHALL-010 LE64(len(P)). Taken to be the length of P in bytes (UTF-8), encoded as 8 bytes little-endian. The tag is the 9 ASCII bytes "AX:OBS:v2" with no terminator, as stated.

10. SHALL-012 parenthetical "SHALL-043 carried over, applied after its normalisation". This refers to a normalisation step from v1 that is not defined here, and it conflicts with SHALL-016 "Admission SHALL NOT normalise, repair or substitute text". Interpretation: no normalisation of any kind is applied; the control-character check runs on the output as given.

11. SHALL-012 and SHALL-016 scope of NFC. SHALL-016 talks about "a string s", while SHALL-012 and the section 9 table apply NFC only to output. Interpretation: NFC (steps 7 and 8) applies to output only, not to identifiers. In this harness steps 7 and 8 are skipped, as VECTOR-FORMAT.md permits, so the choice has no effect on the vectors.

12. SHALL-011 identifier length unit. "At most 256 bytes" is read as the UTF-8 byte length. SHALL-011 lists four conditions but no internal order; since all map to the single reason `identifier`, order does not matter. Invalid UTF-8 in an identifier is rejected as `identifier`, not `output_encoding`.

13. SHALL-012 "valid UTF-8 (well-formed, shortest form, no surrogates, at most U+10FFFF)". Implemented with Python's strict UTF-8 decoder, which rejects overlong forms, encoded surrogates and code points above U+10FFFF. Malformed UTF-8 in output is `output_encoding`.

14. SHALL-006 what counts as an integer. The amendment does not say whether values like 1.0 or 1e2 are integers, or whether booleans are rejected, or with which reason. Admission here receives native integers; any non-integer in an integer slot is rejected as `integer_range`. Not exercised by the vectors.

15. SHALL-006 order inside step 2. Step 2 checks ledger_seq, output_size, then params in the SHALL-004 order (max_tokens, seed, temperature, top_p). Null params are skipped. Since every failure maps to one reason, the inner order only matters for the reason, which is the same; followed as written.

16. SHALL-004 params temperature and top_p "as in v1". The v1 Q16.16 semantics are referenced but not reproduced, and no range narrower than I-JSON is imposed. Interpretation: any integer within plus or minus (2^53 - 1) is admitted (for example -65536).

17. SHALL-014 NULL input with zero length. Only NULL with non-zero length is rejected. Interpretation: NULL with length 0 is treated as the empty input and hashed as SHA-256 of zero bytes. Not exercised.

18. SHALL-005 enum checks. Out-of-enum completion_state or failure_type values are reported as `state_pair`, per the section 9 table. There is no separate enum reason.

19. SHALL-008 note. The note says only `\"`, `\\` and `\n` occur in admitted records. The encoder nevertheless implements the full table (short escapes and lowercase \u00xx for other C0 controls), since P and the stored form must be defined for any string. U+007F, C1 controls and U+2028/U+2029 are written as raw UTF-8, matching RFC 8785.

20. SHALL-004 "lexicographic by UTF-16 code units". The listed order agrees with UTF-16 code unit order for these ASCII names (the code asserts this).

21. Header status. The document says "No implementation is authorised"; this implementation exists only for the conformance-vector exercise of SHALL-018.

22. Section 12 open item 1 (rejected_record_size) and the shape of a rejection record are undefined. Rejections are reported only as the harness line, not as records.

## B. VECTOR-FORMAT.md

23. Mixed notation. The Notation section says `\uXXXX` and `\U0001XXXX` are Python-style escapes, but cases 5, 14 and 17 of Section 1 contain literal non-ASCII characters instead of escapes. A reader copying rendered text could pick up decomposed or look-alike characters (for example U+212B ANGSTROM SIGN for "Å", which is not NFC). I inspected the raw file bytes: "é" is U+00E9, "Å" is U+00C5, "è" is U+00E8, "ω" is U+03C9, and case 17 is U+00A0 U+2028 U+2029. The script encodes these with explicit escapes. Recommend the harness use escapes only.

24. Case 17 is described only by invisible characters plus a parenthetical. The name `del_free_unicode` does not obviously describe the content. Interpretation taken from the parenthetical and the raw bytes.

25. Harness steps 1 and 3 versus SHALL-020 step 9. The harness says validate (steps 1 to 6 and 9), and "otherwise" compute the hashes. But step 9 (record_size) needs the stored record, which needs obs_hash. Interpretation: compute hashes and the stored record after step 6, then apply step 9.

26. BASE "output_size: byte length of output, unless a case sets it". For case 17 of Section 2 (invalid UTF-8), the byte length of the raw bytes (3) is used. Not reached, since output_encoding fails first.

27. Output file termination. "LF line endings" is read as every line, including the last, ending in LF. No trailing blank line. `len=` is printed in decimal.

28. Section 2 case 4 (output_size_over) sets TRUNCATED but keeps output "ok"; integer_range fires at step 2 before output_size, as expected.

29. Section 2 case 22 relies on identifier (step 3) preceding output_encoding (step 5); confirmed.
