#!/usr/bin/env python3
"""
ax-rtm-verify.py
================
Axilog Mechanised Requirements Traceability Matrix Verifier

DVEC: v1.3
Determinism Class: D1 — Strict Deterministic
SRS Coverage: SRS-001-SHALL-003, SRS-001-SHALL-004, SRS-013-SHALL-001,
              SRS-013-SHALL-002, SRS-013-SHALL-004, SRS-013-SHALL-005

Operates as:
  - Pre-commit hook  (fast path, exit 1 on violation)
  - CI gate          (full report, exit 1 on violation)
  - Local audit      (human-readable + JSON output)

Usage:
  python3 scripts/ax-rtm-verify.py [--ci] [--report-dir build/]

Exit codes:
  0  System conformant — all SHALLs anchored, no forbidden patterns
  1  Non-conformant   — violations detected (commit/merge blocked)
  2  Tool error       — SRS or source files unreadable (tool misconfigured)

Author: William Murray · SpeyTech · March 2026
Patent: GB2521625.0
"""

import re
import sys
import os
import json
import glob
import hashlib
import argparse
import datetime
from dataclasses import dataclass, field, asdict
from pathlib import Path
from typing import Optional

# ─────────────────────────────────────────────────────────────────────────────
# DVEC: v1.3 — Tool constants (no magic strings in logic)
# ─────────────────────────────────────────────────────────────────────────────

TOOL_VERSION        = "1.0.0"
DVEC_VERSION        = "v1.3"
SRS_VERSION         = "v0.3"
TOOL_NAME           = "ax-rtm-verify"

# SRS requirement ID pattern — matches SRS-NNN-SHALL-NNN
SRS_PATTERN         = re.compile(r"\bSRS-(\d{3})-SHALL-(\d{3})\b")

# Public function signature patterns (C99)
# Matches: return_type function_name(params) — no body yet
C_FUNC_DECL         = re.compile(
    r"^(?!//|#|\s*\*)"                          # not a comment or preprocessor
    r"(?:(?:static|extern|inline|const)\s+)*"   # optional qualifiers
    r"[\w\s\*]+"                                 # return type
    r"\b(\w+)\s*\("                              # function name (captured)
    r"[^)]*\)",                                  # parameters
    re.MULTILINE
)

# Comment block: looks back from a function for SRS anchors
# Captures everything between /** ... */ or /* ... */ above the signature
COMMENT_BLOCK       = re.compile(r"/\*\*?(.*?)\*/", re.DOTALL)

# DVEC §16 — Forbidden pattern registry
# Format: (pattern, description, severity)
FORBIDDEN_PATTERNS  = [
    # Memory violations
    (re.compile(r"\bmalloc\s*\("),      "dynamic allocation: malloc",          "CRITICAL"),
    (re.compile(r"\bfree\s*\("),        "dynamic allocation: free",            "CRITICAL"),
    (re.compile(r"\brealloc\s*\("),     "dynamic allocation: realloc",         "CRITICAL"),
    (re.compile(r"\bcalloc\s*\("),      "dynamic allocation: calloc",          "CRITICAL"),

    # Floating-point violations
    (re.compile(r"\bfloat\b"),          "floating-point: float",               "CRITICAL"),
    (re.compile(r"\bdouble\b"),         "floating-point: double",              "CRITICAL"),

    # Deferred correctness violations
    (re.compile(r"\bTODO\b"),           "deferred correctness: TODO",          "CRITICAL"),
    (re.compile(r"\bFIXME\b"),          "deferred correctness: FIXME",         "CRITICAL"),
    (re.compile(r"\bHACK\b"),           "deferred correctness: HACK",          "CRITICAL"),
    (re.compile(r"\bOPTIMIZE\b"),       "deferred correctness: OPTIMIZE",      "CRITICAL"),
    (re.compile(r"\?\?\?"),             "deferred correctness: ???",           "CRITICAL"),

    # Time violations (direct system clock access)
    (re.compile(r"\btime\s*\("),        "time oracle: time()",                 "CRITICAL"),
    (re.compile(r"\bclock\s*\("),       "time oracle: clock()",                "CRITICAL"),
    (re.compile(r"\bgettimeofday\s*\("),"time oracle: gettimeofday()",         "CRITICAL"),
    (re.compile(r"\bclock_gettime\s*\("),"time oracle: clock_gettime()",       "CRITICAL"),
]

# Public function declaration pattern — requires a return type token to be present.
# This distinguishes declarations from call sites (e.g. free(p) has no return type).
# Used by extract_unanchored_functions. Stricter than C_FUNC_DECL.
FUNC_DECL_PUBLIC    = re.compile(
    r"^(?!//|#|\s*\*)"
    r"(?:(?:static|extern|inline|const|void|int|uint\w*|int\w*|char|size_t|ax_\w+|ct_\w+)\s+)+"
    r"[\w\s\*]*\b(\w+)\s*\([^)]*\)",
    re.MULTILINE
)

# DVEC §16 — Domain separation registry (machine-readable)
EVIDENCE_TAGS = [
    "AX:STATE:v1",
    "AX:TRANS:v1",
    "AX:OBS:v1",
    "AX:POLICY:v1",
    "AX:PROOF:v1",
]

CHAIN_TAGS = [
    "AX:LEDGER:v1",
    "DVM:LEDGER:v1",
    "DVM:STATE:v1",
    "DVM:INGRESS:v1",
]

# Tag cross-contamination: chain tags must not appear as evidence type values
# and evidence tags must not appear as chain prefixes outside their namespace
CHAIN_TAG_PATTERN   = re.compile(
    r'"(' + "|".join(re.escape(t) for t in CHAIN_TAGS) + r')"'
    r'\s*(?:as|=|:)\s*(?:type|evidence_type)',
    re.IGNORECASE
)

# ─────────────────────────────────────────────────────────────────────────────
# Data types (pure — no side effects)
# ─────────────────────────────────────────────────────────────────────────────

@dataclass
class Requirement:
    """A SHALL statement extracted from an SRS document."""
    req_id:   str       # e.g. SRS-006-SHALL-001
    section:  int       # e.g. 6
    number:   int       # e.g. 1
    source:   str       # file path
    line:     int       # line number in source


@dataclass
class Anchor:
    """A SRS requirement anchor found in C99 source."""
    req_id:   str       # e.g. SRS-006-SHALL-001
    file:     str       # source file path
    line:     int       # line number
    function: str       # associated function name (if resolved)


@dataclass
class ForbiddenMatch:
    """A forbidden pattern found in source."""
    pattern:     str    # description
    severity:    str    # CRITICAL / WARNING
    file:        str
    line:        int
    content:     str    # line content (truncated)


@dataclass
class TagViolation:
    """A domain tag namespace violation."""
    file:        str
    line:        int
    tag:         str
    violation:   str


@dataclass
class UnanchoredFunction:
    """
    A public C99 function with no SRS anchor in its preceding comment block.

    SRS-001-SHALL-003: Every public function SHALL include SRS anchors.
    SRS-001-SHALL-004: No public function SHALL exist without a SHALL statement.
    """
    name:   str     # function name
    file:   str     # source file path
    line:   int     # approximate line number of declaration


@dataclass
class ComplianceViolation:
    """
    A source file missing its mandatory DVEC compliance header.

    SRS-001-SHALL-001: Every module SHALL declare DVEC version.
    SRS-001-SHALL-002: Every module SHALL declare determinism class.
    """
    file:        str
    missing:     str    # "DVEC_HEADER" | "DETERMINISM_CLASS" | "BOTH"


@dataclass
class RTMRow:
    """One row in the Requirements Traceability Matrix."""
    req_id:      str
    anchors:     list        # list of Anchor dicts
    status:      str         # ANCHORED | PENDING | ORPHAN_CODE
    note:        str = ""


@dataclass
class RTMReport:
    """Complete RTM report — the output of one verification run."""
    tool:               str = TOOL_NAME
    tool_version:       str = TOOL_VERSION
    dvec_version:       str = DVEC_VERSION
    srs_version:        str = SRS_VERSION
    timestamp:          str = ""
    conformant:         bool = False
    summary:            dict = field(default_factory=dict)
    matrix:             list = field(default_factory=list)
    forbidden:          list = field(default_factory=list)
    tag_violations:     list = field(default_factory=list)
    unanchored_funcs:   list = field(default_factory=list)
    compliance_viols:   list = field(default_factory=list)
    source_hashes:      dict = field(default_factory=dict)
    tool_self_hash:     str  = ""
    errors:             list = field(default_factory=list)


# ─────────────────────────────────────────────────────────────────────────────
# Extraction — pure functions, no I/O side effects beyond file reads
# ─────────────────────────────────────────────────────────────────────────────

def extract_requirements(srs_files: list[str]) -> tuple[dict, list]:
    """
    SRS-013-SHALL-001: Extract all SHALL statements from SRS documents.

    Returns:
        requirements: dict mapping req_id → Requirement
        errors:       list of error strings
    """
    requirements: dict[str, Requirement] = {}
    errors: list[str] = []

    for srs_path in srs_files:
        try:
            content = Path(srs_path).read_text(encoding="utf-8")
        except OSError as e:
            errors.append(f"Cannot read SRS file {srs_path}: {e}")
            continue

        for line_num, line in enumerate(content.splitlines(), start=1):
            for match in SRS_PATTERN.finditer(line):
                req_id  = match.group(0)
                section = int(match.group(1))
                number  = int(match.group(2))
                if req_id not in requirements:
                    requirements[req_id] = Requirement(
                        req_id  = req_id,
                        section = section,
                        number  = number,
                        source  = srs_path,
                        line    = line_num,
                    )

    return requirements, errors


def extract_anchors(source_files: list[str]) -> tuple[list, list]:
    """
    SRS-001-SHALL-003: Extract SRS anchors from C99 source comment blocks.

    Associates each anchor with the immediately following function signature.

    Returns:
        anchors: list of Anchor
        errors:  list of error strings
    """
    anchors:  list[Anchor] = []
    errors:   list[str]    = []

    for src_path in source_files:
        try:
            content = Path(src_path).read_text(encoding="utf-8")
        except OSError as e:
            errors.append(f"Cannot read source file {src_path}: {e}")
            continue

        lines = content.splitlines()

        # Strategy: walk line by line, track comment blocks,
        # resolve function name from the next non-blank, non-comment line
        # after a closing comment.

        in_comment   = False
        comment_reqs: list[tuple[str, int]] = []  # (req_id, line_num)
        comment_end  = -1

        for line_num, line in enumerate(lines, start=1):
            stripped = line.strip()

            # Comment block open
            if "/*" in stripped and not in_comment:
                in_comment = True
                comment_reqs = []

            # Collect SRS IDs inside comment block
            if in_comment:
                for match in SRS_PATTERN.finditer(stripped):
                    comment_reqs.append((match.group(0), line_num))

            # Comment block close
            if in_comment and "*/" in stripped:
                in_comment   = False
                comment_end  = line_num

                # Look ahead for function signature (within 5 lines)
                func_name = _resolve_function(lines, comment_end, lookahead=5)

                for req_id, req_line in comment_reqs:
                    anchors.append(Anchor(
                        req_id   = req_id,
                        file     = src_path,
                        line     = req_line,
                        function = func_name or "<unresolved>",
                    ))

            # Also catch single-line SRS anchors outside block comments
            # (e.g. // SRS-006-SHALL-001 inline)
            elif not in_comment and "//" in stripped:
                comment_part = stripped[stripped.index("//"):]
                for match in SRS_PATTERN.finditer(comment_part):
                    # Look ahead for function
                    func_name = _resolve_function(lines, line_num, lookahead=3)
                    anchors.append(Anchor(
                        req_id   = match.group(0),
                        file     = src_path,
                        line     = line_num,
                        function = func_name or "<inline>",
                    ))

    return anchors, errors


def _resolve_function(lines: list[str], from_line: int, lookahead: int) -> Optional[str]:
    """
    Look ahead from from_line to find the first C99 function declaration.

    from_line is the 1-indexed line number of the closing */ of the comment
    block. lines is 0-indexed. Therefore lines[from_line] is the line
    immediately AFTER the closing */, which is what we want — the off-by-one
    between 1-indexed line numbers and 0-indexed list access cancels exactly.

    Example:
      line 9  (index 8):  */            ← comment_end = 9
      line 10 (index 9):  void foo(...)  ← lines[9] = lines[comment_end]

    This is intentional. Do not adjust to from_line - 1.
    """
    start = from_line          # 1-indexed line number → 0-indexed list position of next line
    end   = min(start + lookahead, len(lines))

    for i in range(start, end):
        m = C_FUNC_DECL.search(lines[i])
        if m:
            return m.group(1)

    return None


def scan_forbidden(source_files: list[str]) -> list[ForbiddenMatch]:
    """
    DVEC §16: Scan source files for forbidden constructs.
    Skips lines that are entirely within block comments (best-effort).
    """
    matches: list[ForbiddenMatch] = []

    for src_path in source_files:
        try:
            content = Path(src_path).read_text(encoding="utf-8")
        except OSError:
            continue

        lines       = content.splitlines()
        in_comment  = False

        for line_num, line in enumerate(lines, start=1):
            stripped = line.strip()

            has_open  = "/*" in stripped
            has_close = "*/" in stripped

            # Inline block comment on a single code line: code /* comment */
            # Scan only the code portion before the first /*.
            if has_open and has_close and not in_comment:
                code_part = line[:line.index("/*")].split("//")[0]
                for pattern, description, severity in FORBIDDEN_PATTERNS:
                    if pattern.search(code_part):
                        matches.append(ForbiddenMatch(
                            pattern  = description,
                            severity = severity,
                            file     = src_path,
                            line     = line_num,
                            content  = line.rstrip()[:120],
                        ))
                continue

            # Multi-line block comment opens on this line
            if has_open and not in_comment:
                in_comment = True

            # Inside a multi-line block comment — skip
            if in_comment:
                if has_close:
                    in_comment = False
                continue

            # Plain code line — strip // inline comment then scan
            code_part = line.split("//")[0]

            for pattern, description, severity in FORBIDDEN_PATTERNS:
                if pattern.search(code_part):
                    matches.append(ForbiddenMatch(
                        pattern  = description,
                        severity = severity,
                        file     = src_path,
                        line     = line_num,
                        content  = line.rstrip()[:120],
                    ))

    return matches


def scan_tag_violations(source_files: list[str]) -> list[TagViolation]:
    """
    DVEC §4.4: Detect chain tags used as evidence type identifiers.
    SRS-006-SHALL-003: AX:LEDGER:v1 SHALL NOT be used as an evidence type.
    """
    violations: list[TagViolation] = []

    for src_path in source_files:
        try:
            content = Path(src_path).read_text(encoding="utf-8")
        except OSError:
            continue

        lines = content.splitlines()
        for line_num, line in enumerate(lines, start=1):
            m = CHAIN_TAG_PATTERN.search(line)
            if m:
                violations.append(TagViolation(
                    file      = src_path,
                    line      = line_num,
                    tag       = m.group(1),
                    violation = "chain tag used as evidence type identifier",
                ))

    return violations


def extract_unanchored_functions(
    source_files: list[str],
    anchors:      list,
) -> list[UnanchoredFunction]:
    """
    SRS-001-SHALL-003 / SRS-001-SHALL-004:
    Detect public C99 functions that have no SRS anchor in their
    immediately preceding comment block.

    Strategy:
      - Build the set of (file, function_name) pairs that have at least
        one anchor resolved to them.
      - Walk all source files extracting function declarations.
      - Any function not in the anchored set is unanchored.

    Excludes:
      - Static functions (file-private, not public API)
      - Functions in test directories (tests/)
      - Functions named with leading underscore (internal helpers)
    """
    unanchored: list[UnanchoredFunction] = []

    # Build index of anchored (file, func_name) pairs
    anchored_pairs: set[tuple[str, str]] = set()
    for anchor in anchors:
        if anchor.function and anchor.function not in ("<unresolved>", "<inline>"):
            anchored_pairs.add((anchor.file, anchor.function))

    # Also index by function name alone (same function may appear in header
    # and implementation — an anchor in either satisfies the requirement)
    anchored_names: set[str] = {name for _, name in anchored_pairs}

    for src_path in source_files:
        # Skip test directories — test functions are not public API
        if os.sep + "tests" + os.sep in src_path or src_path.endswith("_test.c"):
            continue

        try:
            content = Path(src_path).read_text(encoding="utf-8")
        except OSError:
            continue

        lines = content.splitlines()
        for line_num, line in enumerate(lines, start=1):
            m = FUNC_DECL_PUBLIC.match(line)
            if not m:
                continue

            func_name = m.group(1)

            # Skip internal helpers and static functions
            if func_name.startswith("_"):
                continue
            if "static" in line[:line.index(func_name)]:
                continue

            # Skip if this function name is covered by any anchor
            if func_name in anchored_names:
                continue

            unanchored.append(UnanchoredFunction(
                name = func_name,
                file = src_path,
                line = line_num,
            ))

    return unanchored


# Compliance header patterns (SRS-001-SHALL-001, SRS-001-SHALL-002)
# Accepts both block comment form (/* DVEC: v1.3) and line comment form (// DVEC: v1.3)
# axioma-l0 uses line comment form throughout — both are conformant.
_DVEC_HEADER_PATTERN        = re.compile(r"(?:/\*|//)\s*DVEC:\s*v\d+\.\d+")
_DETERMINISM_CLASS_PATTERN  = re.compile(
    r"AXILOG DETERMINISM:\s*(?:EC-)?D[123]"
)


def scan_compliance_headers(source_files: list[str]) -> list[ComplianceViolation]:
    """
    SRS-001-SHALL-001: Every module SHALL declare DVEC version.
    SRS-001-SHALL-002: Every module SHALL declare determinism class.

    Checks the first 20 lines of each .c and .h file for the mandatory
    compliance header declarations.

    Scope: implementation and header files only. Test stubs and
    .gitkeep placeholders are excluded.
    """
    violations: list[ComplianceViolation] = []

    for src_path in source_files:
        # Only check .c and .h files with actual content
        if not (src_path.endswith(".c") or src_path.endswith(".h")):
            continue

        try:
            content = Path(src_path).read_text(encoding="utf-8")
        except OSError:
            continue

        # Check only the preamble (first 20 lines)
        preamble = "\n".join(content.splitlines()[:20])

        has_dvec        = bool(_DVEC_HEADER_PATTERN.search(preamble))
        has_determinism = bool(_DETERMINISM_CLASS_PATTERN.search(preamble))

        if not has_dvec and not has_determinism:
            violations.append(ComplianceViolation(file=src_path, missing="BOTH"))
        elif not has_dvec:
            violations.append(ComplianceViolation(file=src_path, missing="DVEC_HEADER"))
        elif not has_determinism:
            violations.append(ComplianceViolation(file=src_path, missing="DETERMINISM_CLASS"))

    return violations


def hash_file(path: str) -> str:
    """
    SRS-013-SHALL-007: Toolchain provenance — SHA-256 of source file.
    Deterministic: same file contents → same hash, always.
    """
    h = hashlib.sha256()
    try:
        h.update(Path(path).read_bytes())
        return h.hexdigest()
    except OSError:
        return "unreadable"


# ─────────────────────────────────────────────────────────────────────────────
# RTM construction — pure mapping, no I/O
# ─────────────────────────────────────────────────────────────────────────────

def build_rtm(
    requirements: dict,
    anchors:      list,
) -> list[RTMRow]:
    """
    SRS-013-SHALL-001: Construct the mechanised RTM.

    For each requirement:
      - ANCHORED: at least one code anchor found
      - PENDING:  requirement in SRS, no code anchor yet

    For each anchor whose req_id is NOT in requirements:
      - ORPHAN_CODE: code claims coverage of an unknown requirement
    """
    rows: list[RTMRow] = []

    # Index anchors by req_id
    anchor_index: dict[str, list[Anchor]] = {}
    for anchor in anchors:
        anchor_index.setdefault(anchor.req_id, []).append(anchor)

    # Walk all requirements
    for req_id, req in sorted(requirements.items()):
        found = anchor_index.get(req_id, [])
        if found:
            rows.append(RTMRow(
                req_id  = req_id,
                anchors = [asdict(a) for a in found],
                status  = "ANCHORED",
            ))
        else:
            rows.append(RTMRow(
                req_id  = req_id,
                anchors = [],
                status  = "PENDING",
                note    = "No code anchor — pending implementation",
            ))

    # Walk all anchors — detect orphan code
    req_ids = set(requirements.keys())
    for anchor in anchors:
        if anchor.req_id not in req_ids:
            rows.append(RTMRow(
                req_id  = anchor.req_id,
                anchors = [asdict(anchor)],
                status  = "ORPHAN_CODE",
                note    = f"Anchor references unknown requirement — {anchor.file}:{anchor.line}",
            ))

    return rows


# ─────────────────────────────────────────────────────────────────────────────
# Conformance verdict — pure function
# ─────────────────────────────────────────────────────────────────────────────

def is_conformant(
    rtm_rows:           list[RTMRow],
    forbidden:          list[ForbiddenMatch],
    tag_violations:     list[TagViolation],
    unanchored_funcs:   list[UnanchoredFunction],
    compliance_viols:   list[ComplianceViolation],
    errors:             list[str],
) -> bool:
    """
    Conformance verdict per DVEC-001 v1.3 and SRS-001 v0.3.

    Non-conformant if ANY of:
      - ORPHAN_CODE rows exist (code claims unknown requirement)
      - CRITICAL forbidden patterns detected
      - Chain tag namespace violations detected
      - Unanchored public functions (SRS-001-SHALL-003, SRS-001-SHALL-004)
      - Missing compliance headers (SRS-001-SHALL-001, SRS-001-SHALL-002)
      - Tool errors (SRS or source unreadable)

    PENDING rows (unimplemented requirements) do NOT fail — they
    represent declared open obligations, not violations.

    Note on timestamp: report.timestamp is a wall-clock read for reporting
    purposes only. It is not canonical state. The oracle boundary contract
    (SRS-008-SHALL-003) applies to domain execution, not tooling output.
    """
    if errors:
        return False

    orphan_code    = [r for r in rtm_rows if r.status == "ORPHAN_CODE"]
    critical       = [f for f in forbidden if f.severity == "CRITICAL"]

    if orphan_code or critical or tag_violations or unanchored_funcs or compliance_viols:
        return False

    return True


# ─────────────────────────────────────────────────────────────────────────────
# Report assembly
# ─────────────────────────────────────────────────────────────────────────────

def build_report(
    rtm_rows:           list[RTMRow],
    forbidden:          list[ForbiddenMatch],
    tag_violations:     list[TagViolation],
    unanchored_funcs:   list[UnanchoredFunction],
    compliance_viols:   list[ComplianceViolation],
    source_files:       list[str],
    errors:             list[str],
) -> RTMReport:
    """Assemble the complete RTMReport from all verification results."""

    anchored = [r for r in rtm_rows if r.status == "ANCHORED"]
    pending  = [r for r in rtm_rows if r.status == "PENDING"]
    orphan   = [r for r in rtm_rows if r.status == "ORPHAN_CODE"]
    critical = [f for f in forbidden if f.severity == "CRITICAL"]

    conformant = is_conformant(
        rtm_rows, forbidden, tag_violations,
        unanchored_funcs, compliance_viols, errors
    )

    # SRS-013-SHALL-007: Hash the tool itself — verifier is inside the
    # certification boundary. Tampered tool → different hash → detectable.
    tool_path = os.path.abspath(__file__)

    report = RTMReport(
        timestamp       = datetime.datetime.now(datetime.timezone.utc).isoformat() + "Z",
        conformant      = conformant,
        summary         = {
            "total_requirements":       len(anchored) + len(pending),
            "anchored":                 len(anchored),
            "pending":                  len(pending),
            "orphan_code":              len(orphan),
            "unanchored_functions":     len(unanchored_funcs),
            "compliance_violations":    len(compliance_viols),
            "forbidden_critical":       len(critical),
            "tag_violations":           len(tag_violations),
            "source_files_scanned":     len(source_files),
            "tool_errors":              len(errors),
        },
        matrix          = [asdict(r) for r in rtm_rows],
        forbidden       = [asdict(f) for f in forbidden],
        tag_violations  = [asdict(v) for v in tag_violations],
        unanchored_funcs= [asdict(u) for u in unanchored_funcs],
        compliance_viols= [asdict(c) for c in compliance_viols],
        source_hashes   = {f: hash_file(f) for f in source_files},
        tool_self_hash  = hash_file(tool_path),
        errors          = errors,
    )

    return report


# ─────────────────────────────────────────────────────────────────────────────
# Output — human-readable and machine-readable
# ─────────────────────────────────────────────────────────────────────────────

def print_report(report: RTMReport, verbose: bool = False) -> None:
    """Print human-readable RTM report to stdout."""

    RESET  = "\033[0m"
    RED    = "\033[31m"
    GREEN  = "\033[32m"
    YELLOW = "\033[33m"
    CYAN   = "\033[36m"
    BOLD   = "\033[1m"

    # Use plain output if not a TTY (CI logs)
    is_tty = sys.stdout.isatty()
    def c(colour: str, text: str) -> str:
        return f"{colour}{text}{RESET}" if is_tty else text

    print()
    print(c(BOLD, "╔══════════════════════════════════════════════════════════════╗"))
    print(c(BOLD, f"║  {TOOL_NAME} {TOOL_VERSION:<49}║"))
    print(c(BOLD, f"║  DVEC {DVEC_VERSION} · SRS {SRS_VERSION} · SpeyTech · Patent GB2521625.0  ║"))
    print(c(BOLD, "╚══════════════════════════════════════════════════════════════╝"))
    print()

    s = report.summary
    print(f"  Requirements:   {s['anchored']} anchored / {s['pending']} pending / "
          f"{s['orphan_code']} orphan code")
    print(f"  Unanchored fns: {s['unanchored_functions']}")
    print(f"  Header checks:  {s['compliance_violations']} violations")
    print(f"  Forbidden:      {s['forbidden_critical']} critical violations")
    print(f"  Tag violations: {s['tag_violations']}")
    print(f"  Source files:   {s['source_files_scanned']} scanned")
    print(f"  Tool hash:      {report.tool_self_hash[:16]}...")
    print(f"  Timestamp:      {report.timestamp}")
    print()

    # Verdict
    if report.conformant:
        print(c(GREEN, "  ✓ CONFORMANT — System meets SRS-001 v0.3 traceability requirements"))
    else:
        print(c(RED, "  ✗ NON-CONFORMANT — Violations detected — commit/merge blocked"))

    print()

    # Tool errors
    if report.errors:
        print(c(RED, "  TOOL ERRORS:"))
        for e in report.errors:
            print(f"    {c(RED, '!')} {e}")
        print()

    # Orphan code
    orphans = [r for r in report.matrix if r["status"] == "ORPHAN_CODE"]
    if orphans:
        print(c(RED, "  ORPHAN CODE (SRS-001-SHALL-004 violated):"))
        for r in orphans:
            for a in r["anchors"]:
                print(f"    {c(RED, '✗')} {r['req_id']} → {a['function']}() "
                      f"in {a['file']}:{a['line']}")
        print()

    # Critical forbidden patterns
    critical = [f for f in report.forbidden if f["severity"] == "CRITICAL"]
    if critical:
        print(c(RED, "  FORBIDDEN PATTERNS (DVEC §16 violated):"))
        for f in critical:
            print(f"    {c(RED, '✗')} [{f['severity']}] {f['pattern']}")
            print(f"       {f['file']}:{f['line']}")
            if verbose:
                print(f"       {c(CYAN, f['content'])}")
        print()

    # Tag violations
    if report.tag_violations:
        print(c(RED, "  TAG VIOLATIONS (DVEC §4.4 violated):"))
        for v in report.tag_violations:
            print(f"    {c(RED, '✗')} {v['tag']} — {v['violation']}")
            print(f"       {v['file']}:{v['line']}")
        print()

    # Unanchored public functions (SRS-001-SHALL-003 / SHALL-004)
    if report.unanchored_funcs:
        print(c(RED, "  UNANCHORED PUBLIC FUNCTIONS (SRS-001-SHALL-003/004 violated):"))
        for u in report.unanchored_funcs:
            print(f"    {c(RED, '✗')} {u['name']}() — {u['file']}:{u['line']}")
        print()

    # Compliance header violations (SRS-001-SHALL-001 / SHALL-002)
    if report.compliance_viols:
        print(c(RED, "  MISSING COMPLIANCE HEADERS (SRS-001-SHALL-001/002 violated):"))
        for cv in report.compliance_viols:
            missing_desc = {
                "BOTH":              "missing DVEC version and AXILOG DETERMINISM declaration",
                "DVEC_HEADER":       "missing /* DVEC: vX.Y */ declaration",
                "DETERMINISM_CLASS": "missing /* AXILOG DETERMINISM: D[123] */ declaration",
            }.get(cv["missing"], cv["missing"])
            print(f"    {c(RED, '✗')} {cv['file']}")
            print(f"       {missing_desc}")
        print()

    # Pending requirements (informational — not a failure)
    pending = [r for r in report.matrix if r["status"] == "PENDING"]
    if pending:
        print(c(YELLOW, f"  PENDING REQUIREMENTS ({len(pending)} unimplemented):"))
        for r in pending:
            print(f"    {c(YELLOW, '○')} {r['req_id']} — {r['note']}")
        print()

    # Full matrix (verbose only)
    if verbose:
        anchored = [r for r in report.matrix if r["status"] == "ANCHORED"]
        if anchored:
            print(c(GREEN, "  ANCHORED REQUIREMENTS:"))
            for r in anchored:
                for a in r["anchors"]:
                    print(f"    {c(GREEN, '✓')} {r['req_id']} → "
                          f"{a['function']}() in {a['file']}:{a['line']}")
            print()

    print()


def write_json_report(report: RTMReport, report_dir: str) -> str:
    """Write machine-readable JSON report. Returns path written."""
    Path(report_dir).mkdir(parents=True, exist_ok=True)
    out_path = os.path.join(report_dir, "rtm.json")
    with open(out_path, "w", encoding="utf-8") as f:
        json.dump(asdict(report), f, indent=2)
    return out_path


def write_markdown_report(report: RTMReport, report_dir: str) -> str:
    """Write human-readable Markdown RTM. Returns path written."""
    Path(report_dir).mkdir(parents=True, exist_ok=True)
    out_path = os.path.join(report_dir, "rtm.md")

    lines = [
        "# Axilog Requirements Traceability Matrix",
        "",
        f"**Tool:** {report.tool} {report.tool_version}  ",
        f"**DVEC:** {report.dvec_version}  ",
        f"**SRS:** {report.srs_version}  ",
        f"**Generated:** {report.timestamp}  ",
        f"**Conformant:** {'✓ YES' if report.conformant else '✗ NO'}  ",
        "",
        "## Summary",
        "",
        f"| Metric | Value |",
        f"|--------|-------|",
    ]

    for k, v in report.summary.items():
        lines.append(f"| {k.replace('_', ' ').title()} | {v} |")

    lines += [
        "",
        "## Requirements Traceability Matrix",
        "",
        "| Requirement ID | Status | Function | File | Line |",
        "|----------------|--------|----------|------|------|",
    ]

    for row in report.matrix:
        status = row["status"]
        if row["anchors"]:
            for a in row["anchors"]:
                lines.append(
                    f"| {row['req_id']} | {status} | `{a['function']}()` "
                    f"| {a['file']} | {a['line']} |"
                )
        else:
            note = row.get("note", "")
            lines.append(f"| {row['req_id']} | {status} | — | — | {note} |")

    if report.forbidden:
        lines += [
            "",
            "## Forbidden Pattern Violations",
            "",
            "| Severity | Pattern | File | Line |",
            "|----------|---------|------|------|",
        ]
        for f in report.forbidden:
            lines.append(
                f"| {f['severity']} | {f['pattern']} | {f['file']} | {f['line']} |"
            )

    if report.tag_violations:
        lines += [
            "",
            "## Tag Namespace Violations",
            "",
            "| File | Line | Tag | Violation |",
            "|------|------|-----|-----------|",
        ]
        for v in report.tag_violations:
            lines.append(
                f"| {v['file']} | {v['line']} | {v['tag']} | {v['violation']} |"
            )

    lines += [
        "",
        "## Source File Hashes (SRS-013-SHALL-007)",
        "",
        "| File | SHA-256 |",
        "|------|---------|",
    ]
    for path, digest in sorted(report.source_hashes.items()):
        lines.append(f"| {path} | `{digest}` |")

    if report.unanchored_funcs:
        lines += [
            "",
            "## Unanchored Public Functions (SRS-001-SHALL-003/004)",
            "",
            "| Function | File | Line |",
            "|----------|------|------|",
        ]
        for u in report.unanchored_funcs:
            lines.append(f"| `{u['name']}()` | {u['file']} | {u['line']} |")

    if report.compliance_viols:
        lines += [
            "",
            "## Compliance Header Violations (SRS-001-SHALL-001/002)",
            "",
            "| File | Missing |",
            "|------|---------|",
        ]
        for cv in report.compliance_viols:
            lines.append(f"| {cv['file']} | {cv['missing']} |")

    lines += [
        "",
        "## Tool Provenance (SRS-013-SHALL-007)",
        "",
        f"| Field | Value |",
        f"|-------|-------|",
        f"| Tool | {report.tool} {report.tool_version} |",
        f"| Tool SHA-256 | `{report.tool_self_hash}` |",
        "",
        "---",
        "",
        f"*{report.tool} {report.tool_version} · DVEC {report.dvec_version} · "
        f"SpeyTech · Patent GB2521625.0*",
    ]

    with open(out_path, "w", encoding="utf-8") as f:
        f.write("\n".join(lines) + "\n")

    return out_path


# ─────────────────────────────────────────────────────────────────────────────
# File discovery
# ─────────────────────────────────────────────────────────────────────────────

def discover_srs_files(root: str, srs_root: Optional[str] = None) -> list[str]:
    """
    Discover all SRS markdown files.

    If srs_root is provided, scan that directory directly.
    Otherwise default to {root}/docs/requirements/.

    srs_root accepts absolute paths or paths relative to the working
    directory — enabling cross-repo SRS authority (e.g. axioma-spec
    owns the SRS; all other repos point to it via --srs-root).
    """
    if srs_root:
        search_dir = os.path.abspath(srs_root)
    else:
        search_dir = os.path.join(root, "docs", "requirements")

    pattern = os.path.join(search_dir, "SRS-*.md")
    return sorted(glob.glob(pattern, recursive=False))


def discover_source_files(root: str) -> list[str]:
    """Discover all C99 source and header files under src/ and include/."""
    files = []
    for ext in ("*.c", "*.h"):
        files.extend(glob.glob(os.path.join(root, "src", "**", ext), recursive=True))
        files.extend(glob.glob(os.path.join(root, "include", "**", ext), recursive=True))
    return sorted(files)


# ─────────────────────────────────────────────────────────────────────────────
# Entry point
# ─────────────────────────────────────────────────────────────────────────────

def main() -> int:
    parser = argparse.ArgumentParser(
        description=f"{TOOL_NAME} — Axilog Mechanised RTM Verifier (DVEC {DVEC_VERSION})"
    )
    parser.add_argument(
        "--root",
        default=".",
        help="Repository root directory (default: current directory)",
    )
    parser.add_argument(
        "--report-dir",
        default="build",
        help="Directory for JSON and Markdown report output (default: build/)",
    )
    parser.add_argument(
        "--ci",
        action="store_true",
        help="CI mode: suppress colour, write reports, exit 1 on violation",
    )
    parser.add_argument(
        "--verbose",
        action="store_true",
        help="Verbose: print full anchored requirement list",
    )
    parser.add_argument(
        "--srs-only",
        action="store_true",
        help="Only extract and display requirements (no source scan)",
    )
    parser.add_argument(
        "--srs-root",
        default=None,
        metavar="DIR",
        help=(
            "Directory containing SRS-*.md files. "
            "Defaults to {root}/docs/requirements/. "
            "Use this when the SRS authority lives in a separate repository "
            "(e.g. --srs-root ../axioma-spec/docs/requirements)."
        ),
    )
    parser.add_argument(
        "--version",
        action="version",
        version=f"{TOOL_NAME} {TOOL_VERSION}",
    )

    args = parser.parse_args()
    root = os.path.abspath(args.root)

    # Discover files
    srs_root     = args.srs_root
    srs_files    = discover_srs_files(root, srs_root)
    source_files = discover_source_files(root)

    if not srs_files:
        srs_location = os.path.abspath(srs_root) if srs_root else f"{root}/docs/requirements"
        print(f"ERROR: No SRS files found under {srs_location}", file=sys.stderr)
        print(f"       Expected: SRS-*.md", file=sys.stderr)
        print(f"       Use --srs-root to point at a different directory.", file=sys.stderr)
        return 2

    # Extract requirements
    requirements, req_errors = extract_requirements(srs_files)

    if args.srs_only:
        print(f"\n  {len(requirements)} requirements extracted from {len(srs_files)} SRS file(s):\n")
        for req_id in sorted(requirements):
            print(f"    {req_id}")
        print()
        return 0

    # Extract anchors and scan source
    anchors, anchor_errors     = extract_anchors(source_files)
    forbidden                  = scan_forbidden(source_files)
    tag_violations             = scan_tag_violations(source_files)
    unanchored_funcs           = extract_unanchored_functions(source_files, anchors)
    compliance_viols           = scan_compliance_headers(source_files)

    errors = req_errors + anchor_errors

    # Build RTM and report
    rtm_rows = build_rtm(requirements, anchors)
    report   = build_report(
        rtm_rows, forbidden, tag_violations,
        unanchored_funcs, compliance_viols,
        source_files, errors
    )

    # Output
    print_report(report, verbose=args.verbose or not args.ci)

    if args.ci or args.report_dir:
        json_path = write_json_report(report, args.report_dir)
        md_path   = write_markdown_report(report, args.report_dir)
        print(f"  Reports written:")
        print(f"    {json_path}")
        print(f"    {md_path}")
        print()

    # Exit code
    return 0 if report.conformant else 1


if __name__ == "__main__":
    sys.exit(main())
