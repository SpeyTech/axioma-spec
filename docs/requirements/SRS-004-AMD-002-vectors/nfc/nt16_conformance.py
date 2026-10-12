#!/usr/bin/env python3
# SRS-004-AMD-002 SHALL-017 conformance run against the Unicode 16.0.0
# NormalizationTest.txt, using the independent reference (PyPI unicodedata2
# at exactly 16.0.0).
#
# 1. Verifies the file's SHA-256 equals the digest the owner reported for the
#    copy obtained on the owner's host (DIGEST below). Any other file is
#    refused: nothing is concluded from a checksum alone, and nothing from an
#    unverified file.
# 2. Validates the reference against every line of the file (all five
#    UAX #15 conformance invariants for NFC: c2 == toNFC(c1) == toNFC(c2) ==
#    toNFC(c3), c4 == toNFC(c4) == toNFC(c5)), and checks that every code
#    point not listed in Part 1 is NFC-invariant.
# 3. Derives the SHALL-016 admission verdict for every column string from the
#    file's own columns (independent of the reference), applies the
#    30-non-starter bound with the reference, and requires the reference
#    verdict to agree everywhere.
# 4. Re-checks the targeted vectors in nfc-targeted-u16.txt.
#
# Output: a summary and the SHA-256 of the derived verdict list. Exit 0 only
# if every check passes.
#
# usage: python3 -I nt16_conformance.py NormalizationTest.txt
#        (python with unicodedata2==16.0.0 installed)
import hashlib
import os
import sys

DIGEST = "d811971453e7075e1ad56fb1b301eece5aa80757b81f6156e74a1bfb3ae5ceb1"

import unicodedata2 as U  # noqa: E402

if U.unidata_version != "16.0.0":
    sys.exit("reference must be Unicode 16.0.0, found %s" % U.unidata_version)


def nfc(s):
    return U.normalize("NFC", s)


def over_bound(s):
    run = 0
    for ch in U.normalize("NFD", s):
        run = run + 1 if U.combining(ch) else 0
        if run > 30:
            return True
    return False


def ref_verdict(s):
    if over_bound(s):
        return "REJECTED(nfc_bound)"
    return "ACCEPT" if nfc(s) == s else "REJECTED(not_nfc)"


def cps(field):
    return "".join(chr(int(h, 16)) for h in field.split())


def main(path):
    raw = open(path, "rb").read()
    got = hashlib.sha256(raw).hexdigest()
    if got != DIGEST:
        print("REFUSED: SHA-256 %s does not match the owner-reported digest %s" % (got, DIGEST))
        return 2
    text = raw.decode("utf-8")
    if "NormalizationTest-16.0.0.txt" not in text.splitlines()[0]:
        print("REFUSED: header does not name 16.0.0")
        return 2

    part = None
    lines = invariant_fail = verdict_disagree = bound_hits = 0
    part1 = set()
    derived = []
    for ln in text.splitlines():
        if ln.startswith("@Part"):
            part = ln.split()[0]
            continue
        body = ln.split("#", 1)[0].strip()
        if not body:
            continue
        f = body.split(";")
        c1, c2, c3, c4, c5 = (cps(x) for x in f[:5])
        lines += 1
        if part == "@Part1":
            part1.add(ord(c1))
        if not (nfc(c1) == c2 and nfc(c2) == c2 and nfc(c3) == c2 and nfc(c4) == c4 and nfc(c5) == c4):
            invariant_fail += 1
        for col, s, nf in (("c1", c1, c2), ("c2", c2, c2), ("c3", c3, c2), ("c4", c4, c4), ("c5", c5, c4)):
            if over_bound(s):
                bound_hits += 1
                want = "REJECTED(nfc_bound)"
            else:
                want = "ACCEPT" if s == nf else "REJECTED(not_nfc)"
            derived.append("%s %s %s" % (col, " ".join("%04X" % ord(c) for c in s), want))
            if ref_verdict(s) != want:
                verdict_disagree += 1

    invariant_cp_fail = 0
    for cp in range(0x110000):
        if 0xD800 <= cp <= 0xDFFF or cp in part1:
            continue
        ch = chr(cp)
        if nfc(ch) != ch:
            invariant_cp_fail += 1

    here = os.path.dirname(os.path.abspath(__file__))
    targeted_fail = 0
    for ln in open(os.path.join(here, "nfc-targeted-u16.txt")):
        p = ln.split()
        if ref_verdict("".join(chr(int(h, 16)) for h in p[1:-1])) != p[-1]:
            targeted_fail += 1

    blob = ("\n".join(derived) + "\n").encode()
    print("file sha256 %s (matches owner-reported digest)" % got)
    print("test lines %d, Part1 code points %d" % (lines, len(part1)))
    print("reference NFC invariant failures on file lines %d" % invariant_fail)
    print("code points outside Part1 not NFC-invariant %d" % invariant_cp_fail)
    print("column strings %d, over-bound %d, derived-vs-reference verdict disagreements %d"
          % (len(derived), bound_hits, verdict_disagree))
    print("targeted vector disagreements %d" % targeted_fail)
    print("derived admission verdicts sha256 %s" % hashlib.sha256(blob).hexdigest())
    ok = invariant_fail == 0 and invariant_cp_fail == 0 and verdict_disagree == 0 and targeted_fail == 0
    print("NT16 CONFORMANCE: %s" % ("PASS" if ok else "FAIL"))
    return 0 if ok else 1


if __name__ == "__main__":
    if len(sys.argv) != 2:
        sys.exit("usage: nt16_conformance.py NormalizationTest.txt")
    sys.exit(main(sys.argv[1]))
