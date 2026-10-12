#!/usr/bin/env python3
# Reference generator for SRS-004-AMD-002 section 8 targeted NFC vectors.
# Reference: the PyPI package unicodedata2 at exactly Unicode 16.0.0 (an
# implementation independent of any Axioma code). It verifies the version
# before producing anything. Output: one line per case,
#   <name> <space-separated hex code points> <ACCEPT | REJECTED(not_nfc) | REJECTED(nfc_bound)>
# Verdict rule (SHALL-016): nfc_bound if NFD(s) has a run of more than 30
# consecutive non-starters; else not_nfc if NFC(s) != s; else ACCEPT.
# The bound is checked first so over-bound input is rejected whatever its
# NFC status.
# usage: python3 -I nfc_reference.py   (with unicodedata2==16.0.0 installed)
import sys
import unicodedata2 as U

if U.unidata_version != "16.0.0":
    sys.exit("reference must be Unicode 16.0.0, found %s" % U.unidata_version)

def verdict(s):
    run = longest = 0
    for ch in U.normalize("NFD", s):
        run = run + 1 if U.combining(ch) else 0
        longest = max(longest, run)
    if longest > 30:
        return "REJECTED(nfc_bound)"
    return "ACCEPT" if U.normalize("NFC", s) == s else "REJECTED(not_nfc)"

C = lambda *cps: "".join(chr(c) for c in cps)
CASES = [
    # Latin and singletons
    ("latin_precomposed", C(0x00C5)),
    ("latin_decomposed", C(0x0041, 0x030A)),
    ("angstrom_singleton", C(0x212B)),
    ("cjk_compat_singleton", C(0xF900)),
    ("composable_marks", C(0x0061, 0x0323, 0x0302)),
    ("reordering_ok", C(0x0078, 0x0323, 0x0302)),
    ("reordering_bad", C(0x0078, 0x0302, 0x0323)),
    ("composite_with_extra_mark", C(0x1EAD, 0x0301)),
    ("cyrillic_no_composite", C(0x0430, 0x0301)),
    # Hangul
    ("hangul_syllable", C(0xAC00)),
    ("hangul_lv_jamo", C(0x1100, 0x1161)),
    ("hangul_lvt_jamo", C(0x1100, 0x1161, 0x11A8)),
    ("hangul_lv_plus_t", C(0xAC00, 0x11A8)),
    ("hangul_old_jamo", C(0x1100, 0x1176)),
    # Devanagari
    ("deva_ka_nukta", C(0x0915, 0x093C)),
    ("deva_qa_precomposed", C(0x0958)),
    ("deva_word", C(0x0928, 0x092E, 0x0938, 0x094D, 0x0924, 0x0947)),
    ("deva_nukta_after_vowel_sign", C(0x0915, 0x093F, 0x093C)),
    # Bengali
    ("beng_e_aa", C(0x09C7, 0x09BE)),
    ("beng_o_precomposed", C(0x09CB)),
    ("beng_e_au_length", C(0x09C7, 0x09D7)),
    ("beng_word", C(0x09AC, 0x09BE, 0x0982, 0x09B2, 0x09BE)),
    ("beng_ya_nukta", C(0x09AF, 0x09BC)),
    ("beng_yya_precomposed", C(0x09DF)),
    # Gurmukhi
    ("guru_la_nukta", C(0x0A32, 0x0A3C)),
    ("guru_lla_precomposed", C(0x0A33)),
    # Gujarati
    ("gujr_word", C(0x0A97, 0x0AC1, 0x0A9C, 0x0AB0, 0x0ABE, 0x0AA4, 0x0AC0)),
    # Oriya
    ("orya_e_ai_length", C(0x0B47, 0x0B56)),
    ("orya_ai_precomposed", C(0x0B48)),
    ("orya_e_aa", C(0x0B47, 0x0B3E)),
    # Tamil
    ("taml_ka_virama", C(0x0B95, 0x0BCD)),
    ("taml_e_aa", C(0x0BC6, 0x0BBE)),
    ("taml_o_precomposed", C(0x0BCA)),
    ("taml_au_length", C(0x0BC6, 0x0BD7)),
    # Telugu
    ("telu_e_ai_length", C(0x0C46, 0x0C56)),
    ("telu_ai_precomposed", C(0x0C48)),
    # Kannada
    ("knda_ii_length", C(0x0CBF, 0x0CD5)),
    ("knda_ii_precomposed", C(0x0CC0)),
    ("knda_e_uu", C(0x0CC6, 0x0CC2)),
    # Malayalam
    ("mlym_e_aa", C(0x0D46, 0x0D3E)),
    ("mlym_o_precomposed", C(0x0D4A)),
    ("mlym_word", C(0x0D2E, 0x0D32, 0x0D2F, 0x0D3E, 0x0D33, 0x0D02)),
    # Bound (Stream-Safe, 30 non-starters in NFD). Base "x" has no
    # precomposed form with these marks, so only the bound decides.
    ("bound_30_nonstarters", "x" + C(0x0301) * 30),
    ("bound_31_nonstarters", "x" + C(0x0301) * 31),
    ("bound_30_ordered_mixed_ccc", "x" + C(0x0316) * 15 + C(0x0301) * 15),
    ("bound_30_misordered_mixed_ccc", "x" + C(0x0301, 0x0316) * 15),
    ("bound_31_after_decomposition", C(0x00E1) + C(0x0301) * 30),
    ("bound_reset_by_starter", "x" + C(0x0301) * 30 + "q" + C(0x0301) * 30),
    ("bound_31_nonstarters_and_not_nfc", "a" + C(0x0301) * 31),
    # Unassigned in 15.1, assigned in 16.0 (version sensitivity)
    ("v16_garay_letter", C(0x10D50)),
]

def main():
    for name, s in CASES:
        print("%s %s %s" % (name, " ".join("%04X" % ord(c) for c in s), verdict(s)))
    return 0

if __name__ == "__main__":
    sys.exit(main())
