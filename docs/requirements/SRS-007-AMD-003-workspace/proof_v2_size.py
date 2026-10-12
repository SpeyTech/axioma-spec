#!/usr/bin/env python3
# Worst-case canonical size of an AX:PROOF:v2 record (SRS-007-AMD-003 v0.2,
# SHALL-004), derived from the schema bounds by building the largest
# admissible record text and measuring it. Prints size(n) for selected n
# and the workspace constant AX_PROOF_V2_WORKSPACE_MIN = max over n.
# Escaping per RFC 8785; admissible strings exclude U+0000-U+001F and
# U+007F-U+009F, so the largest expansion is 2 bytes per byte ('"' or '\').
import sys

ENUM_PROOF_TYPE_MAX = max(len(x) for x in [
    "ANCHOR_PUBLICATION", "COMPLIANCE_SUMMARY", "CROSS_LAYER_VERIFY",
    "POLICY_SOUNDNESS", "REPLAY_EQUIVALENCE", "SUBSTRATE_CERT", "WEIGHT_BINDING"])
ENUM_RESULT_MAX = len("INTEGRITY_FAULT")
ENUM_VIOLATION_MAX = max(len(x) for x in [
    "HASH_MISMATCH", "ORDERING", "WEIGHT_MISMATCH", "POLICY_BREACH",
    "TRANSITION_FAULT", "EVIDENCE_MISSING", "REPLAY_MISMATCH", "FALLBACK_OVERFLOW"])
CLAIM_MAX, RULE_MAX = 255, 63
DESC_MAX, KEY_MAX, DIR_MAX = 127, 31, 15
N_MAX = 1024
LEDGER_MAX = 2**53 - 1
H = '"' + "a" * 64 + '"'


def s_worst(nbytes):
    return '"' + '\\"' * nbytes + '"'


def record(n):
    order = "[" + ",".join(str(i) for i in range(n - 1, -1, -1)) + "]"
    members = [
        ("claim", s_worst(CLAIM_MAX)),
        ("closure_alg", '"AX:MERKLE:v2"'),
        ("closure_size", str(n)),
        ("commitment", H),
        ("evidence_closure_root", H),
        ("evidence_order", order),
        ("evidence_refs", "[" + ",".join([H] * n) + "]"),
        ("ledger_seq", str(LEDGER_MAX)),
        ("ordering_metadata", "{" + '"description":' + s_worst(DESC_MAX) + ',"direction":'
         + s_worst(DIR_MAX) + ',"key_field":' + s_worst(KEY_MAX) + "}"),
        ("prev_chain_head", H),
        ("proof_hash", H),
        ("proof_type", '"' + "X" * ENUM_PROOF_TYPE_MAX + '"'),
        ("result", '"' + "X" * ENUM_RESULT_MAX + '"'),
        ("rule_id", s_worst(RULE_MAX)),
        ("schema_version", '"AX:PROOF:v2"'),
        ("violation", '"' + "X" * ENUM_VIOLATION_MAX + '"'),
    ]
    assert [m for m, _ in members] == sorted(m for m, _ in members)
    return "{" + ",".join('"%s":%s' % (k, v) for k, v in members) + "}"


def main():
    sizes = {n: len(record(n).encode()) for n in (1, 2, 9, 10, 99, 100, 192, 999, 1000, 1024)}
    for n, sz in sizes.items():
        print("n=%d worst_canonical_bytes=%d" % (n, sz))
    m = max(len(record(n).encode()) for n in range(1, N_MAX + 1))
    assert m == sizes[1024]
    print("AX_PROOF_V2_WORKSPACE_MIN=%d" % m)
    return 0


if __name__ == "__main__":
    sys.exit(main())
