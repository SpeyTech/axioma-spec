#!/usr/bin/env python3
"""Clean-room implementation of SRS-007-AMD-001 v0.3 (AX:MERKLE:v2) and
Appendix A (AX:MERKLE:v1), emitting the vector file of VECTOR-FORMAT.md v0.3."""

import hashlib
import struct
import sys

TAG = b"AX:MERKLE:v2"
assert TAG == bytes([0x41, 0x58, 0x3A, 0x4D, 0x45, 0x52, 0x4B, 0x4C, 0x45, 0x3A, 0x76, 0x32])
N_MAX = 1024
MAX_PATH = 10


class Rejected(Exception):
    def __init__(self, reason):
        super().__init__(reason)
        self.reason = reason


def sha256(b):
    return hashlib.sha256(b).digest()


def le64(x):
    return struct.pack("<Q", x)


def leaf_hash(d):
    return sha256(b"\x00" + d)


def node_hash(l, r):
    return sha256(b"\x01" + l + r)


def split_point(n):
    # largest power of two strictly less than n (n > 1)
    k = 1
    while k * 2 < n:
        k *= 2
    return k


def mth(D):
    n = len(D)
    if n == 1:
        return leaf_hash(D[0])
    k = split_point(n)
    return node_hash(mth(D[:k]), mth(D[k:]))


def check_input_list(D):
    # SHALL-002, in order: count first, then element length, then ordering.
    n = len(D)
    if n > N_MAX:
        raise Rejected("too_many_refs")
    for d in D:
        if not isinstance(d, (bytes, bytearray)) or len(d) != 32:
            raise Rejected("malformed_reference")
    for i in range(n - 1):
        if not (D[i] < D[i + 1]):
            raise Rejected("input_not_strictly_ascending")


def closure_root(D):
    check_input_list(D)
    n = len(D)
    if n == 0:
        return sha256(TAG + le64(0))
    return sha256(TAG + le64(n) + mth(D))


def path(m, D):
    # SHALL-007 (internal; callers pass a checked list and 0 <= m < n)
    n = len(D)
    if n == 1:
        return []
    k = split_point(n)
    if m < k:
        return path(m, D[:k]) + [mth(D[k:])]
    return path(m - k, D[k:]) + [mth(D[:k])]


def make_path(m, D):
    check_input_list(D)
    n = len(D)
    if not (1 <= n and 0 <= m < n):
        raise Rejected("index_or_size")
    return path(m, D)


def verify(d, proof, R, N):
    m, n, p = proof
    L = len(p)
    # step 0 (lengths only; contents are not read)
    if len(d) != 32 or any(len(x) != 32 for x in p):
        return "REJECTED(malformed_reference)"
    # step 1
    if not (1 <= n <= N_MAX and 0 <= m < n):
        return "REJECTED(index_or_size)"
    # step 1a
    if n != N:
        return "REJECTED(size_mismatch)"
    # step 1b
    if L > MAX_PATH:
        return "REJECTED(path_too_long)"
    # step 2
    fn = m
    sn = n - 1
    r = leaf_hash(d)
    # step 3
    for i in range(L):
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
    # step 4
    if sn != 0:
        return "REJECTED(path_too_short)"
    # step 5
    if sha256(TAG + le64(n) + r) != R:
        return "REJECTED(root_mismatch)"
    return "VERIFIED"


def v1_root(D):
    # Appendix A, frozen v1 construction
    if len(D) == 0:
        return bytes(32)
    if len(D) == 1:
        return D[0]
    level = list(D)
    while len(level) > 1:
        if len(level) % 2 == 1:
            level.append(level[-1])
        level = [sha256(level[2 * i] + level[2 * i + 1]) for i in range(len(level) // 2)]
    return level[0]


def ref(i):
    return sha256(b"AX:MERKLE:v2:vector" + le64(i))


def D_n(n):
    return sorted(ref(i) for i in range(n))


def hx(b):
    return b.hex()


def main():
    out = []
    lists = {}

    def D(n):
        if n not in lists:
            lists[n] = D_n(n)
        return lists[n]

    # Section 1
    for n in (0, 1, 2, 3, 4, 5, 7, 8, 9, 1024):
        Dn = D(n)
        R = closure_root(Dn)
        out.append("ROOT n=%d root=%s" % (n, hx(R)))
        ms = range(n) if n <= 9 else (0, 511, 512, 1023)
        for m in ms:
            p = make_path(m, Dn)
            v = verify(Dn[m], (m, n, p), R, n)
            ps = ",".join(hx(x) for x in p) if p else "-"
            out.append("PROOF n=%d m=%d len=%d path=%s result=%s" % (n, m, len(p), ps, v))

    # Section 2
    D4, D9, D1024 = D(4), D(9), D(1024)
    R4 = closure_root(D4)
    R9 = closure_root(D9)
    R1024 = closure_root(D1024)
    node01 = node_hash(leaf_hash(D4[0]), leaf_hash(D4[1]))
    P39 = make_path(3, D9)
    zero = bytes(32)

    def case(name, d, proof, R, N=None):
        if N is None:
            N = proof[1]
        out.append("REJECT case=%s result=%s" % (name, verify(d, proof, R, N)))

    def build_case(name, lst):
        try:
            closure_root(lst)
            res = "VERIFIED"
        except Rejected as e:
            res = "REJECTED(%s)" % e.reason
        out.append("REJECT case=%s result=%s" % (name, res))

    case("internal_node_as_leaf_short", node01, (0, 4, [mth(D4[2:4])]), R4)
    case("internal_node_as_leaf_n2", node01, (0, 2, [mth(D4[2:4])]), R4)
    case("wrong_index", D9[3], (4, 9, P39), R9)
    case("wrong_size", D9[3], (3, 8, P39), R9)
    case("path_short", D9[3], (3, 9, P39[:-1]), R9)
    case("path_long", D9[3], (3, 9, P39 + [zero]), R9)
    case("path_over_10", D1024[0], (0, 1024, make_path(0, D1024) + [zero]), R1024)
    case("root_for_other_n", D9[3], (3, 9, P39), sha256(TAG + le64(8) + mth(D9)))
    case("index_out_of_range", D9[0], (9, 9, make_path(0, D9)), R9)
    build_case("duplicate_ref", [D9[0], D9[1], D9[1]])
    build_case("non_ascending", [D9[1], D9[0]])
    build_case("too_many_refs", D(1025))
    case("internal_node_as_leaf_full", node01, (0, 4, make_path(0, D4)), R4)
    case("v1_root_presented_as_v2", D4[0], (0, 4, make_path(0, D4)), v1_root(D4))
    case("size_mismatch", D9[3], (3, 9, P39), R9, N=8)

    # Section 3
    for n in (0, 1, 2, 3, 4, 5):
        out.append("V1ROOT n=%d root=%s" % (n, hx(v1_root(D(n)))))

    data = "".join(line + "\n" for line in out)
    sys.stdout.buffer.write(data.encode("ascii"))


if __name__ == "__main__":
    main()
