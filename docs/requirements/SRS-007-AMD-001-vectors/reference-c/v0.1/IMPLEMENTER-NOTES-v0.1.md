# NOTES: merkle_v2.c against SRS-007-AMD-001 v0.1 DRAFT

1. §5 SHALL-008: The text says a path longer than 10 must be rejected "before reading it" but does not name the reason or say where this check sits relative to step 1. Implemented as `REJECTED(path_too_long)`, checked after step 1 and before step 2. The step-3 loop would give the same verdict anyway.
2. §4 SHALL-002 / §6 SHALL-012: The text does not say which check runs first when a list both exceeds N_MAX and is unordered. Implemented `n > N_MAX` first, so nothing is read past the bound. A duplicate reference has no reason code of its own, so it is reported as `input_not_strictly_ascending`.
3. §5 SHALL-008 step 1: The `index_or_size` check runs before the path length is considered, so an invalid (m, n) together with an overlong path gives `index_or_size`.
4. §6 SHALL-011/012: The v1 verdicts (`V1_ROOT_RECOMPUTED`, `V1_PROOF_UNSOUND`) are not defined in enough detail to implement here and are not used by the vectors. They are not implemented.
5. Harness case 1, "internal_node_as_leaf_short": this gives `path_too_short` rather than `root_mismatch`. That is consistent with the text, but the rejection does not exercise the 0x00/0x01 domain separation. Case 2 (n=2) does exercise it and gives `root_mismatch`.
