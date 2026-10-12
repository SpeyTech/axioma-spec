# Vector harness definition, v0.3 (test harness, not normative)

Implement SRS-007-AMD-001 (AMENDMENT.md) from its text only, then emit the vector file below to stdout. Output is ASCII, LF line endings, no trailing spaces, lowercase hex.

## Reference generation
ref_i = SHA-256( ASCII "AX:MERKLE:v2:vector" || LE64(i) ),  i = 0,1,2,...
D_n   = the list [ref_0 .. ref_{n-1}] sorted ascending (bytewise). d_j below always means the j-th element of the SORTED list D_n.

## Section 1: roots and proofs
For n in 0,1,2,3,4,5,7,8,9,1024 (in that order):
  print  "ROOT n=<n> root=<closure_root(D_n) hex>"
  for m in (all 0..n-1 if n <= 9; else 0,511,512,1023 for n=1024), ascending:
    path = PATH(m, D_n)  (SHALL-007)
    v    = verify(d_m, (m, n, path), closure_root(D_n), N = n)  (SHALL-008)
    print "PROOF n=<n> m=<m> len=<len(path)> path=<comma-separated hex, or - if empty> result=<v>"

## Section 2: rejection cases (in this order)
Result text is exactly VERIFIED or REJECTED(<reason>) using the reason names in SHALL-012.
Let R9 = closure_root(D_9), P(m,n) = PATH(m, D_n), and LEAF/NODE/MTH as in the amendment.
Unless a case states otherwise, the expected size N passed to verify equals the proof's own n
(so cases 1-14 exercise the root binding of SHALL-005 on its own; case 15 exercises SHALL-016).
 1 internal_node_as_leaf_short : d = NODE(LEAF(d0),LEAF(d1)) of D_4; proof (0, 4, [MTH(D_4[2:4])]); root closure_root(D_4)
 2 internal_node_as_leaf_n2    : same d and path; proof (0, 2, [MTH(D_4[2:4])]); root closure_root(D_4)
 3 wrong_index                 : d = d3 of D_9; proof (4, 9, P(3,9)); root R9
 4 wrong_size                  : d = d3 of D_9; proof (3, 8, P(3,9)); root R9
 5 path_short                  : d = d3 of D_9; proof (3, 9, P(3,9) without its last element); root R9
 6 path_long                   : d = d3 of D_9; proof (3, 9, P(3,9) + [32 zero bytes]); root R9
 7 path_over_10                : d = d0 of D_1024; proof (0, 1024, P(0,1024) + [32 zero bytes]); root closure_root(D_1024)
 8 root_for_other_n            : d = d3 of D_9; proof (3, 9, P(3,9)); root SHA-256(TAG || LE64(8) || MTH(D_9))
 9 index_out_of_range          : d = d0 of D_9; proof (9, 9, P(0,9)); root R9
10 duplicate_ref               : build closure_root over list [d0, d1, d1] of D_9
11 non_ascending               : build closure_root over list [d1, d0] of D_9
12 too_many_refs               : build closure_root over D_1025
13 internal_node_as_leaf_full  : d = NODE(LEAF(d0),LEAF(d1)) of D_4; proof (0, 4, P(0,4)); root closure_root(D_4)
14 v1_root_presented_as_v2     : d = d0 of D_4; proof (0, 4, P(0,4)); root V1_ROOT(D_4) (Appendix A)
15 size_mismatch               : d = d3 of D_9; proof (3, 9, P(3,9)); root R9; expected size N = 8
print "REJECT case=<name> result=<result>"
For cases 10-12 the result is the construction's rejection (REJECTED(<reason>)); never repair the input.

## Section 3: v1 reference roots (Appendix A)
For n in 0,1,2,3,4,5 (in that order):
  print "V1ROOT n=<n> root=<V1_ROOT(D_n) hex>"
D_n is the same sorted list as in Section 1.
