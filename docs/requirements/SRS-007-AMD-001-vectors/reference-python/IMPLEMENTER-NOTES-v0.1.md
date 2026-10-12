# Ambiguities (literal reading implemented)

1. §5 SHALL-008: "reject a longer path [>10] before reading it" gives no reason name and no position relative to step 1. Implemented after step 1, reason `path_too_long`. Case path_over_10 gives the same verdict either way.
2. §4 SHALL-002 / §6 SHALL-012: duplicates have no own reason. Mapped to `input_not_strictly_ascending`. Check order for a list both too long and unsorted is unspecified; `too_many_refs` checked first.
3. §6 SHALL-012 is titled "Verifier verdicts" but SHALL-002 rejections occur at construction; reused the same names.
4. §3/§4: no reason for a reference that is not 32 bytes (a Python ValueError is raised). SHALL-008 does not check `len(d)`.
5. VECTOR-FORMAT: no section header lines are specified; none emitted.
