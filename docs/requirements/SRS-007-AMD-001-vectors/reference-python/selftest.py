import os, sys
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import merkle_v2 as M

lines = M.section1()
proofs = [l for l in lines if l.startswith("PROOF ")]
assert len(proofs) == 1 + 2 + 3 + 4 + 5 + 7 + 8 + 9 + 4, len(proofs)
for l in proofs:
    assert l.endswith(" result=VERIFIED"), l

rej = M.section2()
assert len(rej) == 12
for l in rej:
    assert "result=REJECTED(" in l and "VERIFIED" not in l, l

D9 = M.D_n(9)
for bad, reason in (([D9[0], D9[1], D9[1]], "input_not_strictly_ascending"),
                    ([D9[1], D9[0]], "input_not_strictly_ascending"),
                    ([D9[0], D9[0]], "input_not_strictly_ascending"),
                    (M.D_n(1025), "too_many_refs")):
    orig = list(bad)
    try:
        M.closure_root(bad)
        raise AssertionError("accepted bad input")
    except M.Rejected as e:
        assert e.reason == reason, e.reason
    assert bad == orig  # not repaired in place
    try:
        M.inclusion_proof(0, bad)
        raise AssertionError("accepted bad input")
    except M.Rejected:
        pass

# sorted/deduplicated version would give a different list: confirm no silent repair path exists
assert M.closure_root([D9[0], D9[1]]) != M.closure_root([D9[0], D9[1], D9[2]])
assert M.closure_root([]) == M.sha256(b"AX:MERKLE:v2" + bytes(8))
print("selftest OK")
