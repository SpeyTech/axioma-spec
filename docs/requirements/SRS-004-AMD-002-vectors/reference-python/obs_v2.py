#!/usr/bin/env python3
"""Clean-room AX:OBS:v2 implementation (SRS-004-AMD-002 v0.2 DRAFT) and vector harness."""
import hashlib
import struct
import sys

TAG = b"AX:OBS:v2"
SCHEMA = "AX:OBS:v2"
I53 = 2 ** 53 - 1
MAX_TOKENS_MAX = 2147483647
AX_MAX_OBS_BYTES = 65536
RRS_MIN = 65537

ABSENT = object()
NULL_INPUT = object()

PAIRS = {
    ("COMPLETE", None),
    ("TRUNCATED", None),
    ("ERROR", "TIMEOUT"),
    ("ERROR", "INVALID_OUTPUT"),
    ("ERROR", "TRANSPORT_ERROR"),
}

ALLOWED_MEMBERS = {
    "completion_state", "failure_type", "input", "input_len", "ledger_seq",
    "model_id", "oracle_id", "output", "output_size", "params",
    "rejected_record_size",
}
PARAM_ORDER = ("max_tokens", "seed", "temperature", "top_p")


class Rejected(Exception):
    def __init__(self, reason):
        Exception.__init__(self, reason)
        self.reason = reason


# ---------- canonical encoding (SHALL-006, SHALL-007, SHALL-008) ----------

SHORT_ESC = {0x22: '\\"', 0x5C: '\\\\', 0x08: '\\b', 0x09: '\\t',
             0x0A: '\\n', 0x0C: '\\f', 0x0D: '\\r'}
HEX = "0123456789abcdef"


def enc_string(s):
    out = ['"']
    for ch in s:
        cp = ord(ch)
        if cp in SHORT_ESC:
            out.append(SHORT_ESC[cp])
        elif cp < 0x20:
            out.append("\\u00" + HEX[cp >> 4] + HEX[cp & 15])
        else:
            out.append(ch)
    out.append('"')
    return "".join(out).encode("utf-8", "strict")


def enc_int(v):
    if type(v) is not int:
        raise TypeError("not an integer")
    if v == 0:
        return b"0"
    neg = v < 0
    n = -v if neg else v
    digits = []
    while n:
        digits.append(chr(48 + n % 10))
        n //= 10
    return (("-" if neg else "") + "".join(reversed(digits))).encode("ascii")


def enc_value(v):
    if v is None:
        return b"null"
    if type(v) is int:
        return enc_int(v)
    if isinstance(v, str):
        return enc_string(v)
    if isinstance(v, list):  # list of (name, value) pairs = object
        return enc_object(v)
    raise TypeError("unsupported value")


def enc_object(pairs):
    parts = [enc_string(k) + b":" + enc_value(v) for k, v in pairs]
    return b"{" + b",".join(parts) + b"}"


def member_list(rec, input_hash, obs_hash):
    """Members in SHALL-004 order; obs_hash omitted when None."""
    m = [
        ("completion_state", rec["completion_state"]),
        ("failure_type", rec["failure_type"]),
        ("input_hash", input_hash),
        ("ledger_seq", rec["ledger_seq"]),
        ("model_id", rec["model_id"]),
    ]
    if obs_hash is not None:
        m.append(("obs_hash", obs_hash))
    m += [
        ("oracle_id", rec["oracle_id"]),
        ("output", rec["output"].decode("utf-8", "strict")),
        ("output_size", rec["output_size"]),
        ("params", [(k, rec["params"][k]) for k in PARAM_ORDER]),
    ]
    if rec.get("rejected_record_size", ABSENT) is not ABSENT:
        m.append(("rejected_record_size", rec["rejected_record_size"]))
    m.append(("schema_version", SCHEMA))
    # self-check: order is lexicographic by UTF-16 code units (ASCII names)
    names = [k for k, _ in m]
    assert names == sorted(names, key=lambda n: [ord(c) for c in n])
    return m


# ---------- hashes (SHALL-009, SHALL-010) ----------

def input_hash_of(data):
    return hashlib.sha256(data).hexdigest()


def obs_hash_of(P):
    return hashlib.sha256(TAG + struct.pack("<Q", len(P)) + P).hexdigest()


# ---------- validation helpers ----------

def strict_utf8(b):
    """Return decoded str or None. Python's strict codec rejects overlong forms,
    surrogates and code points above U+10FFFF."""
    try:
        return b.decode("utf-8", "strict")
    except UnicodeDecodeError:
        return None


def is_c0_c1_del(cp):
    return cp <= 0x1F or 0x7F <= cp <= 0x9F


def check_identifier(v):
    if isinstance(v, str):
        try:
            b = v.encode("utf-8", "strict")
        except UnicodeEncodeError:
            return False
    elif isinstance(v, (bytes, bytearray)):
        b = bytes(v)
    else:
        return False
    if len(b) == 0 or len(b) > 256:
        return False
    s = strict_utf8(b)
    if s is None:
        return False
    for ch in s:
        if is_c0_c1_del(ord(ch)):
            return False
    return True


def in_range(v, lo, hi):
    if v is None:
        return True
    return type(v) is int and lo <= v <= hi


# ---------- admission (SHALL-020) ----------

def admit(rec):
    """Returns stored record bytes and obs_hash, or raises Rejected."""
    # 1. state_pair
    if (rec["completion_state"], rec["failure_type"]) not in PAIRS:
        raise Rejected("state_pair")
    # 2. member_not_permitted
    for k in rec:
        if k not in ALLOWED_MEMBERS:
            raise Rejected("member_not_permitted")
    for k in rec["params"]:
        if k not in PARAM_ORDER:
            raise Rejected("member_not_permitted")
    if rec.get("rejected_record_size", ABSENT) is not ABSENT:
        out = rec["output"]
        if not (rec["completion_state"] == "ERROR"
                and rec["failure_type"] == "INVALID_OUTPUT"
                and isinstance(out, (bytes, bytearray)) and len(out) == 0
                and type(rec["output_size"]) is int and rec["output_size"] == 0):
            raise Rejected("member_not_permitted")
    # 3. integer_range
    if rec["ledger_seq"] is None or not in_range(rec["ledger_seq"], 0, I53):
        raise Rejected("integer_range")
    if rec["output_size"] is None or not in_range(rec["output_size"], 0, I53):
        raise Rejected("integer_range")
    p = rec["params"]
    if not in_range(p.get("max_tokens"), 0, MAX_TOKENS_MAX):
        raise Rejected("integer_range")
    for k in ("seed", "temperature", "top_p"):
        if not in_range(p.get(k), -I53, I53):
            raise Rejected("integer_range")
    if rec.get("rejected_record_size", ABSENT) is not ABSENT:
        v = rec["rejected_record_size"]
        if v is None or not in_range(v, RRS_MIN, I53):
            raise Rejected("integer_range")
    # 4. identifier
    if not check_identifier(rec["model_id"]):
        raise Rejected("identifier")
    if not check_identifier(rec["oracle_id"]):
        raise Rejected("identifier")
    # 5. input
    inp = rec["input"]
    if inp is NULL_INPUT:
        if rec.get("input_len", 0) != 0:
            raise Rejected("input")
        inp = b""
    # 6. output_encoding
    out = bytes(rec["output"])
    s = strict_utf8(out)
    if s is None:
        raise Rejected("output_encoding")
    for ch in s:
        cp = ord(ch)
        if cp != 0x0A and is_c0_c1_del(cp):
            raise Rejected("output_encoding")
    # 7. output_size
    if rec["completion_state"] == "TRUNCATED":
        if not rec["output_size"] > len(out):
            raise Rejected("output_size")
    else:
        if rec["output_size"] != len(out):
            raise Rejected("output_size")
    # 8, 9: NFC checks skipped as permitted by VECTOR-FORMAT.md step 1.
    # 10. record_size
    ih = input_hash_of(inp)
    P = enc_object(member_list(rec, ih, None))
    oh = obs_hash_of(P)
    stored = enc_object(member_list(rec, ih, oh))
    # verifier-side property: P is recoverable by removing obs_hash from stored
    assert stored.replace(b'"obs_hash":"' + oh.encode() + b'",', b"", 1) == P
    if len(stored) > AX_MAX_OBS_BYTES:
        raise Rejected("record_size")
    return stored, oh


# ---------- harness ----------

def base():
    return {
        "completion_state": "COMPLETE",
        "failure_type": None,
        "input": "prompt".encode("utf-8"),
        "ledger_seq": 1,
        "model_id": "m",
        "oracle_id": "o",
        "output": "ok".encode("utf-8"),
        "params": {"max_tokens": None, "seed": None, "temperature": None, "top_p": None},
    }


def make(**ch):
    r = base()
    params = ch.pop("params", {})
    r["params"].update(params)
    explicit_size = "output_size" in ch
    for k, v in ch.items():
        if k == "output" and isinstance(v, str):
            v = v.encode("utf-8")
        if k == "input" and isinstance(v, str):
            v = v.encode("utf-8")
        r[k] = v
    if not explicit_size:
        r["output_size"] = len(r["output"])
    return r


def exact_limit_record(x):
    return make(output='"' * 32000 + "a" * x)


def stored_len_unchecked(rec):
    ih = input_hash_of(rec["input"])
    P = enc_object(member_list(rec, ih, None))
    oh = obs_hash_of(P)
    return len(enc_object(member_list(rec, ih, oh)))


def compute_x():
    l0 = stored_len_unchecked(exact_limit_record(0))
    l1 = stored_len_unchecked(exact_limit_record(1))
    slope = l1 - l0
    assert slope == 1
    x = (AX_MAX_OBS_BYTES - l0) // slope
    assert stored_len_unchecked(exact_limit_record(x)) == AX_MAX_OBS_BYTES
    assert len(str(32000 + x)) == 5
    return x


X = compute_x()

SECTION1 = [
    ("base", make()),
    ("empty_input", make(input="")),
    ("lf_output", make(output="a\nb")),
    ("quote_backslash", make(output="say \"hi\" \\ done")),
    ("non_ascii", make(output="café Å \U0001F600")),
    ("params_full", make(params={"max_tokens": 256, "seed": 9007199254740991,
                                 "temperature": 45875, "top_p": 62259})),
    ("params_negative", make(params={"seed": -9007199254740991,
                                     "temperature": -65536, "top_p": 0})),
    ("truncated", make(completion_state="TRUNCATED", output="abc", output_size=100000)),
    ("error_timeout", make(completion_state="ERROR", failure_type="TIMEOUT", output="")),
    ("error_invalid_output", make(completion_state="ERROR", failure_type="INVALID_OUTPUT", output="")),
    ("error_transport", make(completion_state="ERROR", failure_type="TRANSPORT_ERROR", output="")),
    ("ledger_max", make(ledger_seq=9007199254740991)),
    ("ledger_zero", make(ledger_seq=0)),
    ("identifiers_utf8", make(model_id="modèle-1", oracle_id="ω-oracle")),
    ("identifier_256", make(model_id="m" * 256)),
    ("quote_heavy_fits", make(output='"' * 32000)),
    ("del_free_unicode", make(output="   ")),
    ("max_tokens_zero", make(params={"max_tokens": 0})),
    ("max_tokens_max", make(params={"max_tokens": 2147483647})),
    ("size_rejection_record", make(completion_state="ERROR", failure_type="INVALID_OUTPUT",
                                   output="", rejected_record_size=70000)),
    ("record_exact_limit", exact_limit_record(X)),
]

SECTION2 = [
    ("ledger_over", make(ledger_seq=9007199254740992), "integer_range"),
    ("seed_over", make(params={"seed": 9007199254740992}), "integer_range"),
    ("seed_under", make(params={"seed": -9007199254740992}), "integer_range"),
    ("output_size_over", make(completion_state="TRUNCATED", output_size=9007199254740992), "integer_range"),
    ("complete_with_timeout", make(failure_type="TIMEOUT"), "state_pair"),
    ("error_without_failure", make(completion_state="ERROR", output=""), "state_pair"),
    ("truncated_with_failure", make(completion_state="TRUNCATED", failure_type="INVALID_OUTPUT"), "state_pair"),
    ("unknown_state", make(completion_state="PARTIAL"), "state_pair"),
    ("empty_model_id", make(model_id=""), "identifier"),
    ("control_in_oracle_id", make(oracle_id="o\u0001"), "identifier"),
    ("c1_in_model_id", make(model_id="m\u0085"), "identifier"),
    ("model_id_257", make(model_id="m" * 257), "identifier"),
    ("null_input", make(input=NULL_INPUT, input_len=5), "input"),
    ("control_in_output", make(output="a\u0001b"), "output_encoding"),
    ("cr_in_output", make(output="a\rb"), "output_encoding"),
    ("del_in_output", make(output="x\u007fy"), "output_encoding"),
    ("invalid_utf8_output", make(output=bytes([0x61, 0xFF, 0x62])), "output_encoding"),
    ("size_mismatch", make(output="ok", output_size=3), "output_size"),
    ("truncated_size_under", make(completion_state="TRUNCATED", output="abc", output_size=2), "output_size"),
    ("quote_heavy_over", make(output='"' * 33000), "record_size"),
    ("order_state_before_range", make(failure_type="TIMEOUT", ledger_seq=9007199254740992), "state_pair"),
    ("order_identifier_before_output", make(model_id="", output="a\u0001b"), "identifier"),
    ("max_tokens_negative", make(params={"max_tokens": -1}), "integer_range"),
    ("max_tokens_over", make(params={"max_tokens": 2147483648}), "integer_range"),
    ("rejected_size_on_complete", make(rejected_record_size=70000), "member_not_permitted"),
    ("rejected_size_on_timeout", make(completion_state="ERROR", failure_type="TIMEOUT", output="",
                                      rejected_record_size=70000), "member_not_permitted"),
    ("rejected_size_with_output", make(completion_state="ERROR", failure_type="INVALID_OUTPUT", output="x",
                                       rejected_record_size=70000), "member_not_permitted"),
    ("rejected_size_too_small", make(completion_state="ERROR", failure_type="INVALID_OUTPUT", output="",
                                     rejected_record_size=65536), "integer_range"),
    ("record_exact_limit_plus_one", exact_limit_record(X + 1), "record_size"),
    ("order_member_before_range", make(rejected_record_size=70000, ledger_seq=9007199254740992),
     "member_not_permitted"),
]


def line_for(name, rec):
    try:
        stored, oh = admit(rec)
    except Rejected as e:
        return "REJECT case=%s result=REJECTED(%s)" % (name, e.reason), e.reason
    return ("OBS case=%s len=%d record_sha256=%s obs_hash=%s"
            % (name, len(stored), hashlib.sha256(stored).hexdigest(), oh)), None


def main():
    lines = []
    mismatches = []
    for name, rec in SECTION1:
        ln, reason = line_for(name, rec)
        if reason is not None:
            mismatches.append("%s: expected admission, computed %s" % (name, reason))
        lines.append(ln)
    for name, rec, expected in SECTION2:
        ln, reason = line_for(name, rec)
        if reason != expected:
            mismatches.append("%s: expected %s, computed %s" % (name, expected, reason))
        lines.append(ln)
    sys.stdout.buffer.write(("\n".join(lines) + "\n").encode("ascii"))
    for m in mismatches:
        sys.stderr.write("DISCREPANCY " + m + "\n")
    if "--x" in sys.argv:
        sys.stderr.write("X=%d\n" % X)


if __name__ == "__main__":
    main()
