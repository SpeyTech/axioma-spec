#!/usr/bin/env python3
"""Clean-room implementation of AX:OBS:v2 (SRS-004-AMD-002 v0.1 DRAFT).

Prints the conformance vector file defined by VECTOR-FORMAT.md.
Standard library only; the canonical encoder is hand-written.
"""
import hashlib
import sys

TAG = b"AX:OBS:v2"
SCHEMA_VERSION = "AX:OBS:v2"
I_JSON_MAX = 2 ** 53 - 1
MAX_RECORD_BYTES = 65536
MAX_IDENTIFIER_BYTES = 256

PERMITTED_PAIRS = {
    ("COMPLETE", None),
    ("TRUNCATED", None),
    ("ERROR", "TIMEOUT"),
    ("ERROR", "INVALID_OUTPUT"),
    ("ERROR", "TRANSPORT_ERROR"),
}

PARAM_ORDER = ("max_tokens", "seed", "temperature", "top_p")


class Rejected(Exception):
    def __init__(self, reason):
        super().__init__(reason)
        self.reason = reason


# ---------------------------------------------------------------- encoding

_SHORT_ESCAPES = {
    0x22: '\\"',
    0x5C: "\\\\",
    0x08: "\\b",
    0x09: "\\t",
    0x0A: "\\n",
    0x0C: "\\f",
    0x0D: "\\r",
}
_HEX = "0123456789abcdef"


def encode_string(s):
    """SHALL-008: RFC 8785 string escaping; returns UTF-8 bytes incl. quotes."""
    out = ['"']
    for ch in s:
        cp = ord(ch)
        if cp in _SHORT_ESCAPES:
            out.append(_SHORT_ESCAPES[cp])
        elif cp <= 0x1F:
            out.append("\\u00" + _HEX[cp >> 4] + _HEX[cp & 0xF])
        else:
            out.append(ch)
    out.append('"')
    # Strings reaching here are valid Unicode scalar sequences (validated),
    # so strict UTF-8 encoding cannot fail.
    return "".join(out).encode("utf-8")


def encode_int(n):
    """SHALL-006: decimal, leading '-' for negatives, no '+', no leading zeros."""
    if type(n) is not int:
        raise TypeError("not an integer")
    if n == 0:
        return b"0"
    neg = n < 0
    m = -n if neg else n
    digits = []
    while m:
        digits.append(chr(0x30 + m % 10))
        m //= 10
    return (("-" if neg else "") + "".join(reversed(digits))).encode("ascii")


def encode_value(v):
    if v is None:
        return b"null"
    if isinstance(v, bool):
        raise TypeError("booleans do not occur in AX:OBS:v2")
    if isinstance(v, int):
        return encode_int(v)
    if isinstance(v, str):
        return encode_string(v)
    if isinstance(v, list):  # ordered members: list of (name, value)
        parts = [encode_string(k) + b":" + encode_value(x) for k, x in v]
        return b"{" + b",".join(parts) + b"}"
    raise TypeError("unsupported value %r" % (v,))


def record_members(rec, include_obs_hash):
    """SHALL-004 member order. `rec` holds decoded fields."""
    members = [
        ("completion_state", rec["completion_state"]),
        ("failure_type", rec["failure_type"]),
        ("input_hash", rec["input_hash"]),
        ("ledger_seq", rec["ledger_seq"]),
        ("model_id", rec["model_id"]),
    ]
    if include_obs_hash:
        members.append(("obs_hash", rec["obs_hash"]))
    members += [
        ("oracle_id", rec["oracle_id"]),
        ("output", rec["output"]),
        ("output_size", rec["output_size"]),
        ("params", [(k, rec["params"][k]) for k in PARAM_ORDER]),
        ("schema_version", rec["schema_version"]),
    ]
    # Sanity: the declared order must equal UTF-16 code-unit order (ASCII names).
    names = [n for n, _ in members]
    assert names == sorted(names, key=lambda n: [ord(c) for c in n])
    return members


def canonical(rec, include_obs_hash):
    return encode_value(record_members(rec, include_obs_hash))


# ---------------------------------------------------------------- hashing

def input_hash(input_bytes):
    """SHALL-009: SHA-256 of input bytes, including the empty input."""
    return hashlib.sha256(input_bytes).hexdigest()


def le64(n):
    return n.to_bytes(8, "little")


def obs_hash_of(p_bytes):
    """SHALL-010: SHA-256("AX:OBS:v2" || LE64(len(P)) || P)."""
    return hashlib.sha256(TAG + le64(len(p_bytes)) + p_bytes).hexdigest()


# ---------------------------------------------------------------- validation

def decode_utf8_strict(b):
    """Valid UTF-8: well-formed, shortest form, no surrogates, <= U+10FFFF.
    Python's strict 'utf-8' codec enforces exactly these rules."""
    try:
        return b.decode("utf-8", errors="strict")
    except UnicodeDecodeError:
        return None


def is_c0_or_c1_or_del(cp):
    return cp <= 0x1F or 0x7F <= cp <= 0x9F


def check_integer(v, lo, hi):
    if type(v) is not int or v < lo or v > hi:
        raise Rejected("integer_range")


def check_identifier(raw):
    """SHALL-011. `raw` is bytes."""
    if len(raw) == 0 or len(raw) > MAX_IDENTIFIER_BYTES:
        raise Rejected("identifier")
    s = decode_utf8_strict(raw)
    if s is None:
        raise Rejected("identifier")
    for ch in s:
        if is_c0_or_c1_or_del(ord(ch)):
            raise Rejected("identifier")
    return s


def check_output_encoding(raw):
    """SHALL-012 bullets 1 and 2. `raw` is bytes."""
    s = decode_utf8_strict(raw)
    if s is None:
        raise Rejected("output_encoding")
    for ch in s:
        cp = ord(ch)
        if cp != 0x0A and is_c0_or_c1_or_del(cp):
            raise Rejected("output_encoding")
    return s


def admit(fields):
    """Admission per SHALL-020 (steps 1-6 and 9; 7 and 8 skipped per harness).

    `fields`: completion_state, failure_type (str/None), input (bytes or
    ('NULL', k)), ledger_seq, model_id (bytes), oracle_id (bytes),
    output (bytes), output_size (int), params (dict).
    Returns the stored record bytes and obs_hash, or raises Rejected.
    """
    # 1. state_pair
    if (fields["completion_state"], fields["failure_type"]) not in PERMITTED_PAIRS:
        raise Rejected("state_pair")

    # 2. integer_range: ledger_seq, output_size, then params in order
    check_integer(fields["ledger_seq"], 0, I_JSON_MAX)
    check_integer(fields["output_size"], 0, I_JSON_MAX)
    for k in PARAM_ORDER:
        v = fields["params"][k]
        if v is not None:
            check_integer(v, -I_JSON_MAX, I_JSON_MAX)

    # 3. identifier: model_id then oracle_id
    model_id = check_identifier(fields["model_id"])
    oracle_id = check_identifier(fields["oracle_id"])

    # 4. input
    inp = fields["input"]
    if isinstance(inp, tuple) and inp[0] == "NULL":
        if inp[1] != 0:
            raise Rejected("input")
        inp = b""

    # 5. output_encoding
    output = check_output_encoding(fields["output"])

    # 6. output_size
    out_len = len(fields["output"])
    if fields["completion_state"] in ("COMPLETE", "ERROR"):
        if fields["output_size"] != out_len:
            raise Rejected("output_size")
    else:  # TRUNCATED
        if fields["output_size"] < out_len:
            raise Rejected("output_size")

    # 7, 8: NFC checks. Skipped: VECTOR-FORMAT.md step 1 permits skipping
    # them because every harness output is NFC under any Unicode version.

    rec = {
        "completion_state": fields["completion_state"],
        "failure_type": fields["failure_type"],
        "input_hash": input_hash(inp),
        "ledger_seq": fields["ledger_seq"],
        "model_id": model_id,
        "oracle_id": oracle_id,
        "output": output,
        "output_size": fields["output_size"],
        "params": dict(fields["params"]),
        "schema_version": SCHEMA_VERSION,
    }
    p = canonical(rec, include_obs_hash=False)
    rec["obs_hash"] = obs_hash_of(p)
    stored = canonical(rec, include_obs_hash=True)

    # 9. record_size, on the stored record
    if len(stored) > MAX_RECORD_BYTES:
        raise Rejected("record_size")

    # Self-check of SHALL-010's verifier rule: removing the obs_hash member
    # from the stored bytes yields P exactly.
    member = b'"obs_hash":' + encode_string(rec["obs_hash"]) + b","
    assert stored.count(member) == 1 and stored.replace(member, b"") == p
    return stored, rec["obs_hash"]


# ---------------------------------------------------------------- cases

def u(s):
    return s.encode("utf-8")


def base():
    return {
        "completion_state": "COMPLETE",
        "failure_type": None,
        "input": u("prompt"),
        "ledger_seq": 1,
        "model_id": u("m"),
        "oracle_id": u("o"),
        "output": u("ok"),
        "output_size": None,  # None means: byte length of output
        "params": {"max_tokens": None, "seed": None,
                   "temperature": None, "top_p": None},
    }


def make(changes=None, params=None):
    f = base()
    for k, v in (changes or {}).items():
        f[k] = v
    for k, v in (params or {}).items():
        f["params"][k] = v
    if f["output_size"] is None:
        f["output_size"] = len(f["output"])
    return f


SECTION1 = [
    ("base", make()),
    ("empty_input", make({"input": u("")})),
    ("lf_output", make({"output": u("a\nb")})),
    ("quote_backslash", make({"output": u("say \"hi\" \\ done")})),
    ("non_ascii", make({"output": u("café Å \U0001F600")})),
    ("params_full", make(params={"max_tokens": 256, "seed": 9007199254740991,
                                 "temperature": 45875, "top_p": 62259})),
    ("params_negative", make(params={"seed": -9007199254740991,
                                     "temperature": -65536, "top_p": 0})),
    ("truncated", make({"completion_state": "TRUNCATED", "output": u("abc"),
                        "output_size": 100000})),
    ("error_timeout", make({"completion_state": "ERROR", "failure_type": "TIMEOUT",
                            "output": u("")})),
    ("error_invalid_output", make({"completion_state": "ERROR",
                                   "failure_type": "INVALID_OUTPUT",
                                   "output": u("")})),
    ("error_transport", make({"completion_state": "ERROR",
                              "failure_type": "TRANSPORT_ERROR",
                              "output": u("")})),
    ("ledger_max", make({"ledger_seq": 9007199254740991})),
    ("ledger_zero", make({"ledger_seq": 0})),
    ("identifiers_utf8", make({"model_id": u("modèle-1"),
                               "oracle_id": u("ω-oracle")})),
    ("identifier_256", make({"model_id": u("m" * 256)})),
    ("quote_heavy_fits", make({"output": u("\"" * 32000)})),
    ("del_free_unicode", make({"output": u("   ")})),
]

SECTION2 = [
    ("ledger_over", make({"ledger_seq": 9007199254740992})),
    ("seed_over", make(params={"seed": 9007199254740992})),
    ("seed_under", make(params={"seed": -9007199254740992})),
    ("output_size_over", make({"completion_state": "TRUNCATED",
                               "output_size": 9007199254740992})),
    ("complete_with_timeout", make({"failure_type": "TIMEOUT"})),
    ("error_without_failure", make({"completion_state": "ERROR", "output": u("")})),
    ("truncated_with_failure", make({"completion_state": "TRUNCATED",
                                     "failure_type": "INVALID_OUTPUT"})),
    ("unknown_state", make({"completion_state": "PARTIAL"})),
    ("empty_model_id", make({"model_id": u("")})),
    ("control_in_oracle_id", make({"oracle_id": u("o\u0001")})),
    ("c1_in_model_id", make({"model_id": u("m\u0085")})),
    ("model_id_257", make({"model_id": u("m" * 257)})),
    ("null_input", make({"input": ("NULL", 5)})),
    ("control_in_output", make({"output": u("a\u0001b")})),
    ("cr_in_output", make({"output": u("a\rb")})),
    ("del_in_output", make({"output": u("x\u007fy")})),
    ("invalid_utf8_output", make({"output": bytes([0x61, 0xFF, 0x62])})),
    ("size_mismatch", make({"output": u("ok"), "output_size": 3})),
    ("truncated_size_under", make({"completion_state": "TRUNCATED",
                                   "output": u("abc"), "output_size": 2})),
    ("quote_heavy_over", make({"output": u("\"" * 33000)})),
    ("order_state_before_range", make({"failure_type": "TIMEOUT",
                                       "ledger_seq": 9007199254740992})),
    ("order_identifier_before_output", make({"model_id": u(""),
                                             "output": u("a\u0001b")})),
]


def run_case(name, fields):
    try:
        stored, oh = admit(fields)
    except Rejected as r:
        return "REJECT case=%s result=REJECTED(%s)" % (name, r.reason)
    return "OBS case=%s len=%d record_sha256=%s obs_hash=%s" % (
        name, len(stored), hashlib.sha256(stored).hexdigest(), oh)


def main():
    lines = [run_case(n, f) for n, f in SECTION1 + SECTION2]
    data = ("\n".join(lines) + "\n").encode("ascii")
    sys.stdout.buffer.write(data)


if __name__ == "__main__":
    main()
