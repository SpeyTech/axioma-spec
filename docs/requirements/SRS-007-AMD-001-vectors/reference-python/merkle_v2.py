"""AX:MERKLE:v2 (SRS-007-AMD-001) clean-room implementation + vector emitter."""
import hashlib
import struct
import sys

TAG = b"AX:MERKLE:v2"
N_MAX = 1024
MAX_PATH = 10
assert len(TAG) == 12


def sha256(b):
    return hashlib.sha256(b).digest()


def le64(x):
    return struct.pack("<Q", x)


class Rejected(Exception):
    def __init__(self, reason):
        super().__init__(reason)
        self.reason = reason

    def verdict(self):
        return "REJECTED(%s)" % self.reason


def leaf_hash(d):
    return sha256(b"\x00" + d)


def node_hash(l, r):
    return sha256(b"\x01" + l + r)


def _k(n):
    k = 1
    while k * 2 < n:
        k *= 2
    return k


def check_input(D):
    """SHALL-002: reject, never repair."""
    if len(D) > N_MAX:
        raise Rejected("too_many_refs")
    for d in D:
        if not isinstance(d, (bytes, bytearray)) or len(d) != 32:
            # No reason name exists for this in SHALL-012 (see NOTES.md).
            raise ValueError("reference must be exactly 32 bytes")
    for i in range(1, len(D)):
        if not (bytes(D[i - 1]) < bytes(D[i])):
            raise Rejected("input_not_strictly_ascending")


def mth(D):
    n = len(D)
    if n == 1:
        return leaf_hash(D[0])
    k = _k(n)
    return node_hash(mth(D[0:k]), mth(D[k:n]))


def closure_root(D):
    check_input(D)
    n = len(D)
    if n == 0:
        return sha256(TAG + le64(0))
    return sha256(TAG + le64(n) + mth(D))


def path(m, D):
    n = len(D)
    if n <= 1:
        return []
    k = _k(n)
    if m < k:
        return path(m, D[0:k]) + [mth(D[k:n])]
    return path(m - k, D[k:n]) + [mth(D[0:k])]


def inclusion_proof(m, D):
    check_input(D)
    n = len(D)
    if not (0 <= m < n):
        raise Rejected("index_or_size")
    return (m, n, path(m, D))


def verify(d, proof, R):
    """SHALL-008. Returns verdict string."""
    m, n, p = proof
    # 1.
    if not (1 <= n <= N_MAX and 0 <= m < n):
        return "REJECTED(index_or_size)"
    # Extra rule after the algorithm: reject a path longer than 10 before reading it.
    if len(p) > MAX_PATH:
        return "REJECTED(path_too_long)"
    # 2.
    fn, sn, r = m, n - 1, leaf_hash(d)
    # 3.
    for i in range(len(p)):
        if sn == 0:
            return "REJECTED(path_too_long)"
        pi = p[i]
        if (fn & 1) == 1 or fn == sn:
            r = node_hash(pi, r)
            while (fn & 1) == 0 and fn != 0:
                fn >>= 1
                sn >>= 1
        else:
            r = node_hash(r, pi)
        fn >>= 1
        sn >>= 1
    # 4.
    if sn != 0:
        return "REJECTED(path_too_short)"
    # 5.
    if sha256(TAG + le64(n) + r) != R:
        return "REJECTED(root_mismatch)"
    return "VERIFIED"


# ---------------- vector harness ----------------

def ref(i):
    return sha256(b"AX:MERKLE:v2:vector" + le64(i))


def D_n(n):
    return sorted(ref(i) for i in range(n))


def hx(b):
    return b.hex()


def section1():
    out = []
    for n in (0, 1, 2, 3, 4, 5, 7, 8, 9, 1024):
        D = D_n(n)
        R = closure_root(D)
        out.append("ROOT n=%d root=%s" % (n, hx(R)))
        ms = range(n) if n <= 9 else (0, 511, 512, 1023)
        for m in ms:
            p = path(m, D)
            v = verify(D[m], (m, n, p), R)
            ps = ",".join(hx(x) for x in p) if p else "-"
            out.append("PROOF n=%d m=%d len=%d path=%s result=%s" % (n, m, len(p), ps, v))
    return out


def _build(D):
    try:
        closure_root(D)
        return "VERIFIED"  # unreachable for the cases below; never repaired
    except Rejected as e:
        return e.verdict()


def section2():
    D4, D9, D1024 = D_n(4), D_n(9), D_n(1024)
    R4, R9, R1024 = closure_root(D4), closure_root(D9), closure_root(D1024)
    inner = node_hash(leaf_hash(D4[0]), leaf_hash(D4[1]))
    p4 = [mth(D4[2:4])]
    P39 = path(3, D9)
    cases = [
        ("internal_node_as_leaf_short", verify(inner, (0, 4, p4), R4)),
        ("internal_node_as_leaf_n2", verify(inner, (0, 2, p4), R4)),
        ("wrong_index", verify(D9[3], (4, 9, P39), R9)),
        ("wrong_size", verify(D9[3], (3, 8, P39), R9)),
        ("path_short", verify(D9[3], (3, 9, P39[:-1]), R9)),
        ("path_long", verify(D9[3], (3, 9, P39 + [bytes(32)]), R9)),
        ("path_over_10", verify(D1024[0], (0, 1024, path(0, D1024) + [bytes(32)]), R1024)),
        ("root_for_other_n", verify(D9[3], (3, 9, P39), sha256(TAG + le64(8) + mth(D9)))),
        ("index_out_of_range", verify(D9[0], (9, 9, path(0, D9)), R9)),
        ("duplicate_ref", _build([D9[0], D9[1], D9[1]])),
        ("non_ascending", _build([D9[1], D9[0]])),
        ("too_many_refs", _build(D_n(1025))),
    ]
    return ["REJECT case=%s result=%s" % c for c in cases]


def main():
    sys.stdout.write("\n".join(section1() + section2()) + "\n")


if __name__ == "__main__":
    main()
