# Implementer notes, SRS-007-AMD-001 v0.3 vectors (C implementation)

Implementation: merkle_v2.c (C99, standard library only, own FIPS 180-4 SHA-256 with start-up self-test against "", "abc" and the 448-bit message). Output: vectors-c.txt. Inputs read: AMENDMENT.md and VECTOR-FORMAT.md only.

Each point below names the location, the issue, and the interpretation I chose.

## AMENDMENT.md

1. SHALL-008 step 0 against step 1b (lines 136 and 139). Step 0 requires checking that "every p_i is 32 bytes", which runs before step 1b, and step 1b must reject a long path "before any p_i is read". Taken literally, step 0 already touches every p_i, including the 11th and later in an over-long path. Resolution: step 0 inspects only the length metadata of each supplied element (all within the supplied L), never the element contents. Step 1b then rejects L > 10 before any element content is hashed. If the intent is that no path element of any kind is touched when L > 10, step 0 would need to be split or moved after 1b; that would change the verdict only for an over-long path that also contains a malformed element, which no vector exercises.

2. SHALL-008 step 0 reason name (line 136). The reason for a wrong-length path element is `malformed_reference`, although a path element is a node hash, not a reference. I used `malformed_reference` as written.

3. SHALL-008 step 1 (line 137). "0 ≤ m" is vacuous for an unsigned index. I implemented m and n as unsigned 64-bit, so only m < n and 1 ≤ n ≤ N_MAX are tested.

4. SHALL-008 step 1a and SHALL-016 (lines 126 and 138). The verifier must take N from the carrying record. Since no record schema exists (§9 item 1), N is a separate verifier argument. In the harness N equals the proof's n except in case 15, as VECTOR-FORMAT.md states.

5. SHALL-008 step 3 (lines 141 to 149). The loop is the RFC 9162 §2.1.3.2 algorithm. It is unambiguous as written, but note that in the `fn = sn` branch the inner while runs after node_hash and before the unconditional shift, which I followed exactly. The "if sn = 0" check is evaluated at the top of each iteration, before the element is used.

6. SHALL-012 (lines 183 to 190) says the closed verdict set applies to construction as well, but does not say what verdict a successful construction yields, nor which verdict path generation returns for m ≥ n. Resolution: closure_root and path generation return the SHALL-002 rejection on bad input; a successful construction is reported internally as VERIFIED and simply produces the root or path. Path generation with n = 0 or m ≥ n returns `index_or_size` internally (by analogy with SHALL-008 step 1); no vector exercises this.

7. SHALL-002 (line 79) applies to the "input list" for the root. SHALL-007 (path generation) does not say whether SHALL-002 checks apply to it. Resolution: path generation applies the same SHALL-002 checks before building the path.

8. SHALL-014 (line 85 to 88) says a producer sorts while a verifier must not sort. The harness's closure_root is used both for honest D_n (already sorted by the harness) and for cases 10 and 11 (which must reject). Resolution: closure_root itself never sorts and always applies SHALL-002; sorting is done once, in the harness, when D_n is built (the producer role).

9. SHALL-002 step 1 says "nothing beyond N_MAX is read". For case 12 the harness still has to build D_1025 as test data in memory. Resolution: the harness generates and sorts 1025 refs (test fixture), but closure_root checks n > N_MAX first and returns before touching any element.

10. SHALL-003 table, closure-root row (line 65). The prefix is described as "first byte 0x41 (`A`) followed by the 12-byte TAG", but the 0x41 is the first byte of the 12-byte TAG, not an extra byte. Resolution: the closure-root input is exactly TAG (12 bytes) ‖ LE64(n) ‖ MTH, giving 52 bytes, or TAG ‖ LE64(0), giving 20 bytes, consistent with the stated lengths and with SHALL-005.

11. SHALL-003 consequences (line 73). "A leaf hash cannot equal an internal-node hash ... except by a SHA-256 collision" is a security argument, not a check. Nothing implemented.

12. Appendix A (lines 236 to 245). The appendix is defined "for a strictly ascending list" but does not say whether SHALL-002 checks apply to v1 recomputation (v1 historically had no such checks; that is part of its defect). Resolution: v1_root performs no input checks; it is only called on sorted D_n. The appendix's odd-level rule mutates "level" before pairing; I implemented it as appending a copy of the last element and then hashing pairs of raw 64-byte concatenations with no prefix. V1_ROOT([d_0]) is d_0 unhashed and V1_ROOT([]) is 32 zero bytes.

13. Appendix A pseudo-code uses `len(level)/2 - 1` as an inclusive upper bound. Read as integer division after the odd-length padding, so the range is exact.

14. SHALL-013 (line 212): "Lines carried over from v0.2 SHALL remain byte-identical." I do not have the v0.2 vector file (clean room), so I cannot check this. Cross-implementation comparison is left to the caller.

15. SHALL-013 (line 198) lists "an internal node presented as a leaf, with a short path, at a smaller tree size, and with a path of the correct length" as one item; the harness splits it into cases 1, 2 and 13. I followed the harness.

16. §1 (line 26) mentions identical v1 roots for 5, 6 and 8 leaves "when the last item repeats". This is motivation only; no vector depends on it.

17. SHALL-017, SHALL-011, SHALL-009: record-level dispatch, `unknown_construction`, `V1_ROOT_RECOMPUTED` and `V1_PROOF_UNSOUND` are explicitly outside the vector set (line 190). Not implemented beyond the verdict strings being absent from output.

## VECTOR-FORMAT.md

18. Line 6, reference generation. "ASCII AX:MERKLE:v2:vector" is taken as the 19 bytes of that string with no terminator and no length prefix, followed by LE64(i), giving a 27-byte SHA-256 input.

19. Line 7. D_n is [ref_0 .. ref_{n-1}] sorted, so D_9 is not a prefix of D_1024. Indices d_j refer to the sorted list of that specific n. Followed as written.

20. Line 11 and 15, output formatting. No separators, headers or blank lines are printed for section titles; only ROOT, PROOF, REJECT and V1ROOT lines. Decimal integers have no padding. Each line, including the last, ends in a single LF. `len=` is the path element count. For n = 1 the path field is `-`. n = 0 produces a ROOT line and no PROOF lines.

21. Line 37. The case name printed is the identifier without its number, for example `REJECT case=wrong_index result=REJECTED(root_mismatch)`.

22. Line 22 and 34, cases 1, 2, 13. "d = NODE(LEAF(d0),LEAF(d1)) of D_4" is read as node_hash(leaf_hash(d_0), leaf_hash(d_1)) over sorted D_4, which equals MTH(D_4[0:2]). Slices D[a:b] are half-open (Python style), so D_4[2:4] is [d_2, d_3].

23. Case 2 (line 23) gives proof size n = 2 against closure_root(D_4). Under the general rule N = proof's n = 2, so step 1a passes and the rejection is `root_mismatch`. If N were taken from the D_4 record (N = 4), the result would instead be `size_mismatch`. I followed the harness rule.

24. Case 4 (line 25), wrong_size. With N = proof's n = 8 (harness rule), step 1a passes, and the 4-element P(3,9) runs past sn = 0 at the 4th element, so the verdict is `path_too_long`, not `index_or_size` or `size_mismatch`. The case name suggests a size check, but the actual verdict comes from the path loop. Reported as computed.

25. Case 7 (line 28). P(0,1024) has 10 elements, so the proof has 11 and is rejected at step 1b before the loop. The appended element is 32 zero bytes.

26. Case 8 (line 29). Root is SHA-256(TAG ‖ LE64(8) ‖ MTH(D_9)), with N = 9; result `root_mismatch`.

27. Case 9 (line 30). Proof (9, 9, P(0,9)): step 1 fails on m < n, `index_or_size`.

28. Cases 10 to 12 (lines 31 to 33 and 38). "build closure_root over list" is read as calling the construction (SHALL-002 then SHALL-005) without sorting, and printing its rejection.

29. Case 14 (line 35). V1_ROOT(D_4) is computed with Appendix A and passed as R to the v2 verifier; result `root_mismatch`.

30. Line 3, "lowercase hex": all hex is lowercase, 64 characters per 32-byte value, comma-separated paths with no spaces.

## Build and checks

- `gcc -std=c99 -Wall -Wextra -Werror -pedantic -O2` and `-fsanitize=address,undefined -g` builds are both warning-free, the sanitizer run is clean (no diagnostics), and both produce byte-identical output.
- Every PROOF line verifies; every REJECT line comes from running closure_root or verify on the stated inputs.
