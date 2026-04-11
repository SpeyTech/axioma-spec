# AXIOMA-SDK-STRATEGY-001
## Axioma SDK — Architecture, Language Priority, and Feature Specification

**Version:** 0.4-LOCKED
**Date:** April 2026
**Author:** William Murray, SpeyTech
**Status:** Production Gold — Audit Complete
**Patent:** UK GB2521625.0
**Supersedes:** v0.3-LOCKED

---

## Revision History

| Version | Date | Status | Notes |
|---------|------|--------|-------|
| v0.1 | April 2026 | Superseded | Initial draft |
| v0.2 | April 2026 | Superseded | Closed 8 critical gaps (Review A), 3 strategic opportunities (Review B). Added INV-001 through INV-007, typestate, Web Crypto, PyO3, obs_hash automation |
| v0.3 | April 2026 | Superseded | Second-order hardening + three micro-refinements (ledger_seq constraint, canonical audit surface, no randomness). 10 invariants. External review sign-off achieved |
| v0.4 | April 2026 | Superseded | L0 Epistemic Containment Layer integration. Admission pipeline extended with L0 pre-processing stage. New evidence types. INV-009 extended. Golden vector suite extended. §1.4 PRNG carve-out. No existing invariants modified |
| v0.4-R1 | April 2026 | Superseded | Multi-review closure: SUB-RULE-001 through SUB-RULE-004 (mandatory subtype verification); SRS-EC-SHALL-011 (containment equivalence test) added to golden vector suite and CLI; §12.5 governance probing test strengthened with adversary class declaration; §12.6 split into 12.6a syntactic and 12.6b behavioural; audit record updated |
| v0.4-R2 | April 2026 | Superseded | Third-round closure: SUB-RULE layer discriminant (three-way payload classification); L0_SUBTYPE_MISSING error description tightened to present-but-empty case only; ε_g(Q) budget-scoped in boundary config struct; OQ-004 (marginal fingerprint correlation) referenced; L0-SPEC-001 v1.2 source reference |
| v0.4-LOCKED | April 2026 | Current — Production Gold | Final closure: L0_BUDGET_EXCEEDED error class added; §0 and audit record updated; L0-SPEC-001 v1.3-LOCKED source. Three independent review sign-offs achieved. |

---

## 0. Purpose

This document defines the SDK strategy for Axioma: what it is, who it serves, what it contains, which languages ship first, and what the developer experience looks like.

v0.4 integrates the Epistemic Containment Layer (L0) into the SDK strategy. L0 is specified in L0-SPEC-001 v1.3-LOCKED. All ten invariants (INV-001 through INV-010) from v0.3 are unchanged. L0 adds to the existing specification; it does not modify it.

v0.4-R1 through v0.4-R2 applied three rounds of multi-review closure. v0.4-LOCKED adds `L0_BUDGET_EXCEEDED` to the failure class table (INV-007) and updates the source reference. Three independent review sign-offs confirm mathematical closure.

---

## 1. What the SDK Is (and Is Not)

### 1.1 What It Is

The Axioma SDK is a **commercial, closed-source developer toolkit** that enables engineers to construct, commit, verify, and query Axioma evidence records in their language of choice.

It is the productivity layer over the open-source Axioma specification and the AGPL-licensed substrate libraries.

### 1.2 What It Is Not

The SDK is **not**:

- An arithmetic library (arithmetic lives in libaxilog, C99, AGPL)
- An LLM provider client (the SDK wraps evidence, not API calls)
- A database driver (storage is deployment-specific)
- A compliance report generator (that is L7 tooling, built on top of the SDK)
- A replacement for reading the spec (the SDK implements the spec, not hides it)
- An L0 execution engine (the SDK records L0 evidence; L0 domain logic runs in the framework layer)

### 1.3 Commercial Model

```
OPEN (AGPL-3.0)
  libaxilog           — L1 substrate, C99, Q16.16 arithmetic
  axioma-spec         — Framework specification
  axioma-oracle       — L3 reference implementation
  axioma-agent        — L5 reference implementation
  axioma-audit        — L6 reference implementation
  axioma-policy       — L4 reference implementation
  axioma-governance   — L7 reference implementation
  axioma-verify       — Verification tooling
  certifiable-*       — Nine production repos
  axioma-l0           — L0 Epistemic Containment Layer (C99, AGPL)

COMMERCIAL (Proprietary — SpeyTech Commercial License)
  axioma-sdk/rust     — Rust SDK
  axioma-sdk/ts       — TypeScript SDK
  axioma-sdk/python   — Python SDK (later)
  libdvm              — Patented C99 SDK (Patent GB2521625.0)
```

### 1.4 Core Invariants

**The SDK guarantees that all constructed evidence records are mathematically closed under their defined operations.**

This is not a design goal. It is the contract.

**The canonical payload is the sole audit surface.** No external metadata (headers, transport artefacts, logging side-channels) influences commitment.

**The SDK SHALL NOT use any source of randomness.** No PRNG, no entropy, no nondeterministic seed. All outputs are fully determined by inputs.

> **L0 PRNG boundary (v0.4):** The SDK SHALL NOT execute PRNG operations. L0 epistemic containment PRNG is framework-layer domain logic, specified in L0-SPEC-001 §3 and implemented in `axioma-l0` (C99, AGPL). The SDK records L0 evidence (seed commitments, simulation parameter records, canonicalisation records) but does not generate or consume pseudorandom values. A caller integrating L0 runs the PRNG in the framework layer and presents completed evidence records to the SDK for commitment. This boundary is architectural, not advisory.

---

## 2. Consumer Profiles

### 2.1 Profile A — Safety-Critical Engineers (Track A)

**Who:** Aerospace, medical device, automotive teams. C99 shops.
**What they need:** libaxilog headers, static library, golden reference tooling.
**Language:** C99 only. No wrapper. The DVEC contract is the API.
**Product:** libaxilog (AGPL) + libdvm (commercial, patented).
**SDK relevance:** Low.

### 2.2 Profile B — Governance Engineers (Track B)

**Who:** Enterprise AI teams deploying LLMs in regulated environments. EU AI Act, FCA PS22/3, ISO 42001.
**What they need:** Construct AX:OBS:v1 records, commit to the ledger, evaluate policies, verify chains.
**Language:** Rust, TypeScript, or Python.
**Product:** axioma-sdk (commercial).
**SDK relevance:** High. Primary SDK consumer.

**v0.4 addition — Epistemic governance engineers:** A subset of Profile B requiring L0 integration for frontier model deployments where regime-conditioned computation selection is a credible threat. These engineers additionally need to construct L0 evidence records (seed commitments, simulation parameter records, canonicalisation records) and verify L0 session integrity.

### 2.3 Profile C — Compliance Teams

**Who:** Regulatory affairs, audit, legal.
**What they need:** Report generation, anchor verification, evidence querying.
**Language:** Not applicable — CLI tools and dashboards.
**Product:** axioma-governance tooling (built on top of the SDK).
**SDK relevance:** Indirect.

---

## 3. Language Priority

### 3.1 Priority Order

| Priority | Language | Rationale | Target Consumer |
|----------|----------|-----------|--------------------|
| **Tier 1** | **Rust** | Safe FFI to libaxilog C99. BTreeMap for JCS. Typestate pipeline enforcement. Maps to Semper Victus / aerospace / defence. | Profile B (safety-adjacent governance) |
| **Tier 2** | **TypeScript** | Cloud-native enterprise. Web Crypto API (Node/Deno/Bun/browser). Oracle Gateway MVP. EU AI Act Article 9. | Profile B (cloud governance) |
| **Tier 3** | **Python** | PyO3 wrapper around Rust core for JCS. No independent Python JCS implementation. | Profile B (enterprise AI, later) |

### 3.2 Python Strategy

Python SDK SHALL NOT implement JCS independently. Core canonicalisation and commitment logic compiled from Rust via PyO3. Python layer provides ergonomic wrappers. Golden vectors prove equivalence, but the implementation is shared, not reimplemented.

---

## 4. SDK Invariants (Non-Negotiable)

INV-001 through INV-010 are unchanged from v0.3. They are reproduced here in full for completeness. No invariant has been modified. L0 integration adds new capabilities within the existing invariant set; it does not require any invariant to be weakened or qualified.

### 4.1 Byte Identity Guarantee

**INV-001:** For any evidence record, canonical bytes SHALL be bit-identical across all SDK languages and all executions.

This is the core invariant. Everything else exists to enforce it.

**Verification:** Cross-language golden vector suite. Every commit. No exceptions.

### 4.2 DVEC Version Binding

**INV-002:** Every SDK build SHALL declare and enforce:

```
DVEC_VERSION = "1.3"
SDK_VERSION  = "0.1.0"
CONFORMANCE  = "FULL"
```

The SDK SHALL refuse to operate on evidence payloads with incompatible DVEC versions.

**Verification:** Version check at initialisation. Incompatible payloads rejected with `DVEC_VERSION_MISMATCH` error.

### 4.3 Time Oracle Contract

**INV-003:** The SDK SHALL NOT access system time directly. All timestamps SHALL be obtained through a Time Oracle interface.

```rust
pub trait TimeOracle {
    fn now_ns(&self) -> u64;
}
```

**Requirements:**
- Monotonicity enforced: `now_ns()` returns values strictly greater than the previous call
- Rollback rejected with `TIME_ROLLBACK` error
- All timestamps in evidence records sourced from this interface
- No `SystemTime::now()`, `Date.now()`, or `time.time_ns()` in SDK code outside the default oracle implementation

**v0.3 Clarification — Time is not an ordering source:**

> **Timestamps are informational. Ledger sequence is the only ordering authority.**

The `timestamp_ns` field in evidence records is metadata for human consumption. It SHALL NOT be used for ordering evidence records, replay sequencing, or ordering verification. Use `ledger_seq` for all ordering.

**v0.4 L0 note — Evaluation epoch:** The `evaluation_epoch` parameter in L0 session initialisation (L0-SPEC-001 §3.3) is sourced from the Time Oracle per this invariant. L0 does not access system time. The evaluation epoch is injected as an immutable value and committed to L6 as part of the session initialisation record.

**Verification:** Static analysis scan for direct time access. Replay tests with injected clock. Ordering verification tests SHALL NOT reference `timestamp_ns`.

### 4.4 Numeric Encoding Contract

**INV-004:** All numeric values in canonical evidence payloads SHALL be representable as integers within declared bounds.

```json
// CORRECT
"temperature": 45875

// FORBIDDEN
"temperature": 0.7
```

**v0.3 Hardening — Bounds enforcement:**

All numeric fields SHALL have declared bounds. The SDK SHALL reject out-of-range values at construction.

| Field Type | Bound | Enforcement |
|------------|-------|-------------|
| Q16.16 values | [-2147483648, 2147483647] (INT32 range) | Builder rejects out-of-range |
| `ledger_seq` | [0, 2^64-1] | u64 type enforces |
| `max_tokens` | [0, 2^32-1] or null | Builder rejects negative |
| Token counts | [0, 2^32-1] | Builder rejects negative |
| `latency_ms` | [0, 2^32-1] | Builder rejects negative |

**v0.4 additions — L0 numeric fields:**

| Field Type | Bound | Enforcement |
|------------|-------|-------------|
| PCG-64 seed | [0, 2^64-1] | u64 type enforces |
| PCG-64 stream | [0, 2^64-1] | u64 type enforces |
| Fingerprint bin count | [16, 256] | Builder rejects out-of-range |
| Simulation parameters | Per L0-SPEC-001 §5.2 | Builder rejects out-of-range |
| Baseline ε bounds | [0, 2^32-1] | Builder rejects negative |

**Verification:** Property tests at builder boundaries with out-of-range values.

### 4.5 String Validation Boundary

**INV-005:** The SDK SHALL reject invalid strings at the API boundary, not during canonicalisation.

Construction SHALL fail immediately if input contains:
- Non-UTF-8 byte sequences
- Non-NFC normalised text
- Disallowed control characters (U+0000–U+001F excluding U+000A)
- CRLF or CR line endings (must be LF only)

**Verification:** Property tests with invalid input at every builder entry point.

**v0.3 Hardening — Cross-language normalisation equivalence:**

Golden vectors SHALL include multi-language Unicode edge cases demonstrating pre/post NFC normalisation equivalence. Added to `jcs_edge_cases.json`.

### 4.6 Deterministic Ordering Contract

**INV-006:** All collections that influence commitment SHALL have a defined, explicit ordering rule.

| Collection | Ordering Rule |
|------------|---------------|
| JSON object keys | Lexicographic (RFC 8785) |
| `evidence_refs` array | Lexicographic by hash (ascending) before Merkle build |
| `policy_seqs` array | Ascending by `ledger_seq` |
| `policy_results` array | Corresponding order to `policy_seqs` |
| Merkle tree leaves | Lexicographic sort of input hashes |

**v0.3 Hardening — Internal enforcement guarantee:**

> All ordering SHALL occur inside the SDK. Callers SHALL NOT be responsible for ordering.

Builders SHALL accept unsorted input, normalise ordering internally, and never expose ordering responsibility to call sites.

**Verification:** Golden vectors include ordering-sensitive test cases with deliberately unsorted input.

### 4.7 Failure Semantics Model

**INV-007:** All SDK failures SHALL be explicit, typed, and non-recoverable unless declared.

**Failure classes:**

| Class | Description | Recovery |
|-------|-------------|----------|
| `CONSTRUCTION_ERROR` | Missing required field, invalid input, bounds violation | Fix input, retry |
| `CANONICALISATION_ERROR` | JCS failure | Bug report |
| `COMMITMENT_MISMATCH` | Recomputed hash differs from declared | Non-recoverable |
| `CHAIN_INTEGRITY_FAILURE` | Chain break detected | Non-recoverable |
| `MERKLE_INVALID` | Proof does not verify against root | Non-recoverable |
| `DVEC_VERSION_MISMATCH` | Incompatible evidence version | Non-recoverable |
| `TIME_ROLLBACK` | Timestamp ≤ previous timestamp | Non-recoverable |
| `ORDERING_VIOLATION` | Collection ordering invariant breached | Non-recoverable |
| `L0_COMMITMENT_ORDER` | L0 PRNG output requested before seed committed | Non-recoverable |
| `L0_CONTAINMENT_FAILURE` | L0 state history deviation exceeds ε | Non-recoverable |
| `L0_SUBTYPE_MISSING` | AX:STATE:v1 or AX:OBS:v1 record carries a present-but-empty l0_subtype field (neither a valid vocabulary value nor null/absent) | Non-recoverable |
| `L0_BUDGET_EXCEEDED` | l0_admit() called when calls_processed ≥ query_budget_Q; session locked | Non-recoverable |

**v0.3 Hardening — Deterministic failure outputs:**

> Identical failure conditions SHALL produce identical error outputs across all SDK languages.

Failure records SHALL be canonical:

```json
{
  "error": "L0_COMMITMENT_ORDER",
  "session_id": "...",
  "evidence_type": "AX:STATE:v1"
}
```

Same bytes. Same structure. Every language.

**v0.4-R2 — Subtype layer discriminant (SUB-RULE-001 through SUB-RULE-004):**

The SDK enforces the layer discriminant from L0-SPEC-001 §11.1 (v1.2). Records are classified by payload, not by provenance:

- **L0-domain record:** `l0_subtype` is non-null and a valid vocabulary value → verified using subtype-specific rules
- **Non-L0-domain record:** `l0_subtype` is null or absent → verified as an ordinary evidence record; no L0 rules applied
- **Malformed record:** `l0_subtype` is present but empty → `L0_SUBTYPE_MISSING` error; non-conformant

This three-way classification is payload-deterministic. No context tracking or provenance inference is required. A verifier reading only the record payload can apply the correct rule.

The SDK enforces this at two points:

1. **Construction time (SUB-RULE-003):** The `epistemic` module SHALL set `l0_subtype` to a valid vocabulary value on every record it constructs. Non-epistemic builders SHALL set `l0_subtype: null` or omit the field. A present-but-empty `l0_subtype` at construction time returns `CONSTRUCTION_ERROR` with detail `"l0_subtype must be null, absent, or a valid vocabulary value"`.

2. **Verification time (SUB-RULE-001, SUB-RULE-002):** `verify_invariants` and `axioma verify invariants` apply the three-way classification to every `AX:STATE:v1` and `AX:OBS:v1` record. Only present-but-empty `l0_subtype` fields trigger `L0_SUBTYPE_MISSING`. Null or absent fields on non-L0 records are valid and do not trigger an error.

The declared `l0_subtype` vocabulary is:

| Value | Tag | Description |
|---|---|---|
| `L0:SESSION` | `AX:STATE:v1` | L0 session initialisation record |
| `L0:SEED` | `AX:STATE:v1` | PCG-64 seed commitment |
| `L0:FINGERPRINT` | `AX:STATE:v1` | Production fingerprint record |
| `L0:BASELINE` | `AX:STATE:v1` | Operational baseline record |
| `L0:SIM` | `AX:OBS:v1` | Simulation parameter record |
| `L0:CANON` | `AX:OBS:v1` | Canonicalisation record |

**Verification:** Golden vectors include failure test cases with expected error output, including L0-specific failure classes and `L0_SUBTYPE_MISSING`.

### 4.8 Observation Hash Atomicity

**INV-008 (v0.3):** The `obs_hash` computation SHALL be atomic and non-observable.

The builder SHALL NOT expose:
- The pre-hash canonical form (without obs_hash)
- Any intermediate record state during the two-step computation
- Logging of intermediate canonical bytes

**Verification:** API review confirming no public method returns intermediate observation state.

### 4.9 Execution Mode Audit Visibility

**INV-009 (v0.3, extended in v0.4):** The SDK execution mode SHALL be recorded in evidence metadata.

When constructing evidence records, the SDK SHALL include:

```json
"sdk_context": {
  "sdk_version": "0.1.0",
  "dvec_version": "1.3",
  "execution_mode": "STRICT",
  "epistemic_mode": "EC-D1"
}
```

**v0.4 extension — `epistemic_mode`:**

The `epistemic_mode` field SHALL be included in all evidence records produced by the SDK. Its value SHALL be one of:

| Value | Meaning |
|-------|---------|
| `"EC-D1"` | L0 session is active; PRNG seed committed; all normalisation under EC-D1 determinism |
| `"NONE"` | No L0 session active; epistemic containment not applied |

The `epistemic_mode` field is part of the canonical payload. Mode changes are detectable in the audit trail. A record produced during an active L0 session MUST carry `"EC-D1"`. A record produced without an L0 session MUST carry `"NONE"`. The SDK SHALL reject construction of a record that claims `"EC-D1"` without a committed L0 session identifier.

**Verification:** Golden vectors include EC-D1 and NONE mode records with different expected hashes. SDK rejects `"EC-D1"` claim without session ID.

### 4.10 Tenant Boundary Isolation

**INV-010 (v0.3):** In multi-tenant environments, the SDK SHALL enforce logical isolation between tenant evidence streams.

```rust
pub struct TenantBoundary {
    tenant_id: String,
    chain: LedgerChain,
    time_oracle: Box<dyn TimeOracle>,
}
```

**Requirements:**
- Evidence record builders SHALL be scoped to a tenant boundary
- A `LedgerChain` instance SHALL serve exactly one tenant
- Cross-tenant evidence construction SHALL be rejected at the API level
- Tenant ID SHALL be included in the `sdk_context` metadata

**Verification:** Integration tests confirming cross-tenant operations are rejected.

---

## 5. SDK Feature Specification

### 5.1 Core Features (All Languages)

#### 5.1.1 Evidence Record Builders

Typed constructors for all evidence types with compile-time (Rust/TS) or immediate runtime (Python) enforcement of required fields.

| Evidence Type | Tag | SDK Module |
|---------------|-----|------------|
| State Commitment | `AX:STATE:v1` | `records::state` |
| Transition Commitment | `AX:TRANS:v1` | `records::transition` |
| Observation Record | `AX:OBS:v1` | `records::observation` |
| Policy Assertion | `AX:POLICY:v1` | `records::policy` |
| Verification Proof | `AX:PROOF:v1` | `records::proof` |

**v0.4 additions — L0 evidence types:**

| Evidence Type | Tag | SDK Module | Description |
|---|---|---|---|
| L0 Session Record | `AX:STATE:v1` (subtype `L0:SESSION`) | `epistemic::session` | L0 session initialisation: seed, stream IDs, session ID, evaluation epoch |
| L0 Seed Commitment | `AX:STATE:v1` (subtype `L0:SEED`) | `epistemic::seed` | PCG-64 seed commitment before first PRNG output |
| L0 Fingerprint Record | `AX:STATE:v1` (subtype `L0:FINGERPRINT`) | `epistemic::fingerprint` | Production fingerprint identity and parameters |
| L0 Simulation Parameters | `AX:OBS:v1` (subtype `L0:SIM`) | `epistemic::simulation` | Per-call simulation parameters generated from fingerprint replay |
| L0 Canonicalisation Record | `AX:OBS:v1` (subtype `L0:CANON`) | `epistemic::canon` | Prompt canonicalisation output and metadata |
| L0 Baseline Record | `AX:STATE:v1` (subtype `L0:BASELINE`) | `epistemic::baseline` | Operational baseline parameters and ε bounds |

All L0 evidence types use existing `AX:STATE:v1` and `AX:OBS:v1` tags with an `l0_subtype` field. No new top-level evidence type tags are introduced. This preserves the domain separation registry (DVEC-001 §4.4) without modification.

#### 5.1.2 Oracle Admission Pipeline (Extended in v0.4)

**v0.3 pipeline:**

```
Unvalidated → Validated → Canonicalised → Committed
```

**v0.4 pipeline (L0 pre-processing stage):**

```
Unvalidated → L0Transformed → Validated → Canonicalised → Committed
```

The `L0Transformed` stage is inserted between `Unvalidated` and `Validated`. It is present only when an L0 session is active (`epistemic_mode = "EC-D1"`). When no L0 session is active, the pipeline is unchanged from v0.3 and `L0Transformed` is a pass-through.

**Typestate pattern (Rust, extended):**

```rust
struct Unvalidated;
struct L0Transformed;
struct Validated;
struct Canonicalised;
struct Committed;

struct Observation<State> { /* fields */ }

impl Observation<Unvalidated> {
    // With L0 session: applies canonicalisation, returns L0Transformed
    fn l0_transform(
        self,
        session: &L0Session,
    ) -> Result<Observation<L0Transformed>, AxiomaError>;

    // Without L0 session: direct to Validated (v0.3 behaviour)
    fn validate(self) -> Result<Observation<Validated>, AxiomaError>;
}

impl Observation<L0Transformed> {
    fn validate(self) -> Result<Observation<Validated>, AxiomaError>;
}

impl Observation<Validated> {
    fn canonicalise(self) -> Result<Observation<Canonicalised>, AxiomaError>;
}

impl Observation<Canonicalised> {
    fn commit(self) -> Result<Observation<Committed>, AxiomaError>;
}
```

The `l0_transform` method:
1. Applies prompt canonicalisation (L0-SPEC-001 §4) to the prompt field
2. Records a `L0:CANON` evidence record
3. Injects simulation parameters from the active session
4. Records a `L0:SIM` evidence record
5. Returns the transformed observation ready for validation

The `L0Session` type is a handle to a committed L0 session. The SDK does not implement the PRNG; it receives the session handle which carries the committed session identifier. The framework layer (axioma-l0, C99) executes L0 domain logic and presents the SDK with completed evidence for commitment.

**v0.4 admission pipeline table:**

| Step | Operation | INV | L0 Active? |
|------|-----------|-----|-----------|
| Capture | Receive raw oracle output | — | Both |
| L0 Transform | Prompt canonicalisation, simulation injection | INV-009 | EC-D1 only |
| L0 Record | Commit L0:CANON and L0:SIM evidence | INV-001, INV-009 | EC-D1 only |
| Validate | UTF-8 + NFC, size bound, control chars, bounds | INV-004, INV-005 | Both |
| Canonicalise | RFC 8785, line normalisation, minimal escaping | INV-001 | Both |
| Commit | AX:OBS:v1, obs_hash (atomic, INV-008), commitment | INV-001, INV-008 | Both |
| Timestamp | From Time Oracle, monotonicity enforced | INV-003 | Both |
| Metadata | sdk_context included (version, mode, epistemic_mode) | INV-009 | Both |
| Return | Typed, committed observation record | — | Both |

#### 5.1.3 RFC 8785 (JCS) Canonicalisation

Unchanged from v0.3.

- Lexicographic key ordering (Unicode codepoint order)
- Deterministic number serialisation
- Minimal string escaping
- UTF-8 output with NFC normalisation
- Line ending normalisation: LF only

#### 5.1.4 Domain-Separated SHA-256 Commitment

Unchanged from v0.3:

```
commit(evidence) = SHA-256(tag || LE64(|payload|) || payload)
```

#### 5.1.5 Ledger Chain Operations

Unchanged from v0.3. L0 session records, seed commitments, fingerprint records, simulation parameters, and canonicalisation records are all committed via the standard chain append operation. L0 does not require new chain operations.

#### 5.1.6 Q16.16 Fixed-Point Utilities

Unchanged from v0.3. L0 does not use Q16.16 arithmetic. All L0 numeric parameters use uint32_t with explicit multipliers (L0-SPEC-001 §5.2).

#### 5.1.7 Policy Evaluation

Unchanged from v0.3.

#### 5.1.8 Evidence Closure Merkle Tree

Unchanged from v0.3.

#### 5.1.9 Chain Verification and Replay

Unchanged from v0.3. L0 evidence records (L0:SESSION, L0:SEED, L0:FINGERPRINT, L0:SIM, L0:CANON, L0:BASELINE) are included in chain verification as standard `AX:STATE:v1` and `AX:OBS:v1` records. No special-case verification is required.

#### 5.1.10 Proof Construction

Unchanged from v0.3.

#### 5.1.11 Fault Inspector

Unchanged from v0.3.

#### 5.1.12 Invariant Verifier

Extended in v0.4 to include L0 invariants, and in v0.4-R1 to enforce SUB-RULE-001 through SUB-RULE-004:

```bash
axioma verify invariants ./record.json
# INV-001 Byte Identity:          PASS
# INV-002 DVEC Version:           PASS
# INV-003 Time Oracle:            PASS (timestamp_ns present, ledger_seq ordered)
# INV-004 Numeric Bounds:         PASS
# INV-005 String Validity:        PASS
# INV-006 Ordering:               PASS
# INV-007 Failure Semantics:      N/A (no failure)
# INV-008 obs_hash:               PASS
# INV-009 Execution Mode:         PASS (sdk_context present, epistemic_mode: EC-D1)
# INV-010 Tenant Boundary:        PASS
# L0 Seed Commitment Order:       PASS (seed committed before session L0:SIM records)
# L0 Session Integrity:           PASS (session_id consistent across L0 records)
# L0 Canonical Form:              PASS (canon record present for this observation)
# L0 Subtype Coverage (SUB-001):  PASS (all EC-D1 records carry valid l0_subtype)
# L0 Subtype Vocabulary (SUB-002): PASS (all l0_subtype values in declared vocabulary)
```

For a session with a missing subtype:

```bash
axioma verify invariants ./record-missing-subtype.json
# L0 Subtype Coverage (SUB-001):  FAIL — AX:OBS:v1 record at ledger_seq 14 missing l0_subtype
# Error: L0_SUBTYPE_MISSING
```

The `verify_invariants` API returns a per-check result set. All L0 checks are mandatory when `epistemic_mode` is `"EC-D1"`. They are skipped (reported as N/A) when `epistemic_mode` is `"NONE"`.

### 5.2 Epistemic Module (v0.4 Addition)

The `epistemic` module is the SDK interface for L0 evidence construction. It does not implement L0 logic — that lives in `axioma-l0` (C99, AGPL). The module provides builders for L0 evidence types and the `L0Session` handle.

```rust
// axioma-sdk/rust/src/epistemic/
mod session;       // L0Session handle, session record builder
mod seed;          // Seed commitment record builder
mod fingerprint;   // Fingerprint record builder
mod simulation;    // Simulation parameter record builder
mod canon;         // Canonicalisation record builder
mod baseline;      // Baseline record builder
mod config;        // EpistemicMode enum, L0Config
mod divergence;    // RegimeDivergenceTest builder
```

**`L0Session` handle:**

```rust
pub struct L0Session {
    session_id: [u8; 16],
    seed_commitment: [u8; 32],    // SHA-256 of committed seed record
    epistemic_mode: EpistemicMode,
    committed: bool,
}

pub enum EpistemicMode {
    ECD1,
    None,
}
```

The `L0Session` is constructed from a committed L0 session initialisation record. The SDK verifies that `committed = true` before allowing `L0Transformed` state transitions. If `committed = false`, the `l0_transform` call returns `L0_COMMITMENT_ORDER` error (INV-007).

**Key types:**

```rust
pub struct L0SeedRecord {
    pub session_id: [u8; 16],
    pub seed_hex: String,          // hex-encoded uint64
    pub stream_hex: String,        // hex-encoded uint64
    pub l6_chain_head_hex: String, // 32-byte chain head at seed derivation time
    pub evaluation_epoch: u64,     // from Time Oracle
}

pub struct L0SimRecord {
    pub session_id: [u8; 16],
    pub call_index: u64,           // monotonically increasing within session
    pub latency_ms_x100: u32,
    pub failure_per_1000: u32,
    pub jitter_ms_x100: u32,
    pub bg_task_rate_x10: u32,
    pub ctx_len_tokens: u32,
    pub fingerprint_identity_hex: String,
}

pub struct L0CanonRecord {
    pub session_id: [u8; 16],
    pub call_index: u64,
    pub input_length_bytes: u32,
    pub output_length_bytes: u32,
    pub segments_count: u32,
    pub metadata_stripped: bool,
    pub whitespace_changed: bool,
    pub nfc_applied: bool,
    pub canon_hash_hex: String,    // SHA-256 of canonical output
}
```

All fields are integers or hex strings. No float. Bounds per INV-004.

**`TransformationPipeline`:**

```rust
pub struct TransformationPipeline {
    session: L0Session,
}

impl TransformationPipeline {
    pub fn new(session: L0Session) -> Result<Self, AxiomaError>;

    pub fn transform(
        &mut self,
        raw_prompt: &str,
        raw_sim_params: &L0SimParams,  // from axioma-l0 C99 layer
    ) -> Result<TransformationResult, AxiomaError>;
}

pub struct TransformationResult {
    pub canonical_prompt: String,
    pub canon_record: L0CanonRecord,
    pub sim_record: L0SimRecord,
}
```

The `transform` method takes a raw prompt and pre-computed simulation parameters from the C99 L0 layer. It does not run the PRNG or the canonicalisation automaton — those are C99 domain logic. It constructs the evidence records for commitment and returns the canonical prompt for use in the admission pipeline.

**`SimulationConfig`:**

```rust
pub struct SimulationConfig {
    pub fingerprint_identity: [u8; 32],
    pub bin_counts: [u8; 5],           // per dimension
    pub baseline_ref: [u8; 32],        // SHA-256 of committed baseline record
}
```

**`RegimeDivergenceTest`:**

```rust
pub struct RegimeDivergenceTest {
    pub task_count: u32,
    pub eval_results: Vec<[u8; 32]>,   // commitment hashes of eval outputs
    pub norm_results: Vec<[u8; 32]>,   // commitment hashes of normalised outputs
    pub significance_level_x10000: u32, // α × 10000 (e.g. 500 = α 0.05)
}

impl RegimeDivergenceTest {
    pub fn build_proof(&self) -> Result<DivergenceProof, AxiomaError>;
}

pub struct DivergenceProof {
    pub passed: bool,
    pub p_value_x10000: u32,    // p-value × 10000
    pub evidence: AX_PROOF_V1, // AX:PROOF:v1 record for commitment
}
```

### 5.3 Rust-Specific Features

| Feature | Description |
|---------|-------------|
| `libaxilog` FFI | Safe bindings via `bindgen` |
| `axioma-l0` FFI | Safe bindings to C99 L0 layer (v0.4) |
| `serde` integration | Derive-based serialisation |
| `BTreeMap` for JCS | Guaranteed lexicographic key ordering |
| Typestate pipeline | Compile-time admission enforcement (extended with `L0Transformed`) |
| `ct_fault_flags_t` mapping | `From<libaxilog::ct_fault_flags_t>` |
| `TenantBoundary` | Logical isolation per INV-010 |
| `epistemic` module | L0 evidence builders (v0.4) |

**`no_std`:** Deferred to v2.

### 5.4 TypeScript-Specific Features

| Feature | Description |
|---------|-------------|
| **Web Crypto API** | `crypto.subtle.digest()` — Node/Deno/Bun/browser |
| Strict types | Branded types |
| Zod schemas | Runtime validation + OpenAPI schema export |
| ESM + CJS | Dual module support |
| `BigInt` for ledger_seq | 64-bit without precision loss |
| `epistemic` module | L0 evidence builders (v0.4) |

### 5.5 Python-Specific Features (Deferred)

| Feature | Description |
|---------|-------------|
| **PyO3 Rust core** | JCS and commitment from Rust SDK |
| Dataclass records | Frozen dataclasses |
| Full mypy strict | Type annotations throughout |

---

## 6. Determinism Mode

```rust
let sdk = AxiomaSDK::new(SdkConfig {
    strict_mode: true,
    time_oracle: Box::new(InjectableClock::new()),
    tenant_id: "tenant-001".to_string(),
    epistemic_mode: EpistemicMode::ECD1,   // v0.4 addition
    l0_session: Some(committed_session),   // v0.4 addition — None if no L0
})?;
```

When `epistemic_mode: EpistemicMode::ECD1`:
- `l0_session` MUST be `Some(committed_session)` where `committed_session.committed = true`
- All observation records produced by this SDK instance carry `epistemic_mode: "EC-D1"`
- The admission pipeline includes the `L0Transformed` stage
- `l0_transform` is callable on `Observation<Unvalidated>`

When `epistemic_mode: EpistemicMode::None`:
- `l0_session` SHALL be `None`
- All observation records carry `epistemic_mode: "NONE"`
- The admission pipeline is unchanged from v0.3
- Calling `l0_transform` returns `CONSTRUCTION_ERROR`

**Execution mode is recorded in evidence (INV-009).** Mode changes are detectable in the audit trail.

---

## 7. Developer Experience — Command Examples

### 7.1 Rust SDK (Extended)

```rust
use axioma_sdk::{
    AxiomaSDK, SdkConfig,
    oracle::AdmissionPipeline,
    chain::LedgerChain,
    policy::PolicySet,
    proof::ProofBuilder,
    q16::Q16,
    time::SystemTimeOracle,
    tenant::TenantBoundary,
    epistemic::{L0Session, TransformationPipeline, SimulationConfig, EpistemicMode},
};

// Obtain committed L0 session from axioma-l0 C99 layer (framework responsibility)
// The SDK receives the session handle; it does not create it.
let l0_session = L0Session::from_committed_record(&session_init_record)?;

// Initialise SDK with L0 session
let sdk = AxiomaSDK::new(SdkConfig {
    dvec_version: "1.3",
    strict_mode: true,
    time_oracle: Box::new(SystemTimeOracle::new()),
    tenant_id: "tenant-001".to_string(),
    epistemic_mode: EpistemicMode::ECD1,
    l0_session: Some(l0_session),
})?;

// Transformation pipeline (L0 domain logic executed in axioma-l0 C99 layer,
// results presented to SDK for evidence commitment)
let raw_sim_params = l0_c99_layer.next_sim_params()?;  // axioma-l0 FFI call
let pipeline = TransformationPipeline::new(sdk.l0_session().clone())?;
let transformed = pipeline.transform(&raw_prompt, &raw_sim_params)?;

// Commit L0 evidence records
chain.append(&sdk.commit_l0_canon(&transformed.canon_record)?)?;
chain.append(&sdk.commit_l0_sim(&transformed.sim_record)?)?;

// Oracle admission (typestate enforced, includes L0Transformed stage)
let admitted = sdk.admission_pipeline()
    .oracle_id("azure-openai-prod-westeurope")
    .model_id("gpt-4-turbo-2024-04-09")
    .params(OracleParams { /* ... */ })
    .build()?
    .admit(
        &transformed.canonical_prompt,  // L0-canonicalised prompt
        &raw_llm_response,
        next_ledger_seq,
    )?;

// Chain, policy, proof — unchanged from v0.3
let mut chain = LedgerChain::load("chain.dat")?;
chain.append(&admitted.commitment)?;

// Invariant verification — includes L0 checks
let inv_result = sdk.verify_invariants(&admitted.record)?;
assert!(inv_result.all_pass());
```

### 7.2 CLI (Extended)

```bash
# All v0.3 commands unchanged

# L0 session inspection
axioma l0 session inspect ./session-record.json
# Session ID:       a1b2c3d4e5f60708090a0b0c0d0e0f10
# Seed commitment:  f3e2d1...
# Epistemic mode:   EC-D1
# Committed:        true
# Chain head at init: 7bb0d791...

# L0 seed replay verification
axioma l0 seed replay --session ./session-record.json --trace ./sim-trace.json
# Replaying 47 simulation parameter sets...
# Bit-identical: PASS

# L0 canonicalisation verification
axioma l0 canon verify --input ./raw-prompt.txt --canon ./canon-record.json
# Canonical form: PASS
# Idempotent:     PASS
# Metadata stripped: 3 elements
# Whitespace changed: true

# Transformation sensitivity test (SRS-EC-SHALL-008)
axioma l0 diverge sensitivity --raw-results ./raw-eval-outputs/ --norm-results ./norm-outputs/ --alpha 500
# Tasks: 500
# Test: transformation sensitivity (raw eval vs L0-normalised)
# p-value: 0.3821 (× 10000: 3821)
# Threshold: 0.05 (× 10000: 500)
# Result: PASS — no task-content distortion detected

# Containment equivalence test (SRS-EC-SHALL-011)
axioma l0 diverge equivalence --norm-results ./norm-outputs/ --prod-results ./prod-sample/ --alpha 500
# Tasks: 500
# Test: containment equivalence (L0-normalised vs production)
# p-value: 0.7204 (× 10000: 7204)
# Threshold: 0.05 (× 10000: 500)
# Result: PASS — model cannot distinguish normalised evaluation from production

# Subtype verification (SUB-RULE-001 through SUB-RULE-004)
axioma l0 subtype verify ./session-chain.dat
# Records inspected: 127
# L0 records identified: 34
# Subtype field present: 34/34
# Subtype values valid:  34/34
# Result: PASS (SUB-RULE-001, SUB-RULE-002)

# Verify invariants including L0 and SUB-RULE checks
axioma verify invariants ./record.json
# INV-001 Byte Identity:           PASS
# INV-009 Execution Mode:          PASS (epistemic_mode: EC-D1)
# L0 Seed Commitment Order:        PASS
# L0 Session Integrity:            PASS
# L0 Canonical Form:               PASS
# L0 Subtype Coverage (SUB-001):   PASS
# L0 Subtype Vocabulary (SUB-002): PASS
```

---

## 8. Cross-Language Golden Vector Suite

### 8.1 Requirement

**Non-negotiable.** No SDK ships until it passes the golden vector suite.

### 8.2 Structure (Extended)

```
golden/
  vectors.json                       <- master vector file
  obs_canonical.json
  obs_commitment.hex
  obs_hash_circular.json
  policy_canonical.json
  policy_commitment.hex
  proof_canonical.json
  proof_commitment.hex
  trans_canonical.json
  trans_commitment.hex
  state_canonical.json
  state_commitment.hex
  merkle_4leaf.hex
  merkle_7leaf.hex
  chain_genesis.hex
  chain_3append.hex
  jcs_edge_cases.json
  ordering_sensitive.json
  failure_vectors.json
  mode_strict.json
  mode_default.json
  mode_ec_d1.json                    <- v0.4: EC-D1 mode record with expected hash
  mode_none.json                     <- v0.4: NONE mode record with expected hash
  l0/
    pcg_init_seed0_stream1.hex       <- L0-PV-001: 10 outputs from known state
    pcg_init_known_seed.hex          <- L0-PV-002: 10 outputs from L0_STREAM_LATENCY
    pcg_stream_separation.hex        <- L0-PV-003: two streams, same seed, different outputs
    pcg_fingerprint_replay.hex       <- L0-PV-004: 4-bin histogram replay values
    canon_plain_task.json            <- L0-CV-001: plain task, no metadata
    canon_with_metadata.json         <- L0-CV-002: evaluator attribution stripped
    canon_markdown_headers.json      <- L0-CV-003: heading segmentation
    canon_unicode_nfc.json           <- L0-CV-004: NFC normalisation
    canon_idempotent.json            <- L0-CV-005: idempotence test
    canon_harness_elements.json      <- L0-CV-006: harness header/footer stripped
    canon_multisegment.json          <- L0-CV-007: examples and segments
    l0_session_record.json           <- example committed session record
    l0_seed_commitment.json          <- example seed commitment record
    l0_sim_record.json               <- example simulation parameter record
    l0_canon_record.json             <- example canonicalisation record
    l0_failure_commitment_order.json <- expected error: L0_COMMITMENT_ORDER
    l0_failure_containment.json      <- expected error: L0_CONTAINMENT_FAILURE
    l0_failure_subtype_missing.json  <- v0.4-R1: expected error: L0_SUBTYPE_MISSING (SUB-RULE-002)
    l0_divergence_proof.json         <- v0.4-R1: SRS-EC-SHALL-011 containment equivalence test proof record
    l0_adversary_class_v_b.json      <- v0.4-R1: governance probing test at class V-B
    l0_adversary_class_v_c.json      <- v0.4-R1: governance probing test at class V-C (stub — deployment-specific)
```

### 8.3 Derivation

C99 reference implementation (`axioma-l0`, AGPL) is the source of truth for all L0 golden vectors. All other vectors: C99 reference implementation in libaxilog / axioma-oracle.

| Source | Golden Reference |
|--------|--------------------|
| axioma-audit E0 | `0976582f90120f7c10263221aef8f0666156f465fc46cd48ef9aa2d6a1ed390c` |
| axioma-audit L0 | `7bb0d791697306ce2f1cc5df0bcdf66d810d6af9425aa380b352a62453a5ec7b` |
| axioma-audit L3 | `0a6b796ca38fe030c7108e15551a05ee1628392e9a88af94ccf840b8d4605d3e` |

L0 golden references will be established from the `axioma-l0` C99 reference implementation once it is authored. They are not pre-computable here.

### 8.4 License

Golden vectors are MIT. Anyone can build their own SDK and test it.

---

## 9. Repository Structure (Extended)

```
axioma-sdk/
  golden/
    (all vector files per §8.2)
    l0/
      (all L0 vector files per §8.2)
  rust/
    Cargo.toml
    src/
      lib.rs
      config.rs
      invariants.rs
      time.rs
      tenant.rs
      records/
        mod.rs
        observation.rs
        transition.rs
        policy.rs
        proof.rs
        state.rs
      epistemic/                             <- v0.4 addition
        mod.rs
        session.rs                           <- L0Session handle
        seed.rs                              <- Seed commitment builder
        fingerprint.rs                       <- Fingerprint record builder
        simulation.rs                        <- Simulation parameter builder
        canon.rs                             <- Canonicalisation record builder
        baseline.rs                          <- Baseline record builder
        config.rs                            <- EpistemicMode, L0Config
        divergence.rs                        <- TransformationSensitivityTest + ContainmentEquivalenceTest (v0.4-R1: SHALL-008/011 split)
        pipeline.rs                          <- TransformationPipeline
        subtype.rs                           <- v0.4-R1: SubtypeVerifier, SUB-RULE-001 through SUB-RULE-004
      jcs.rs
      commitment.rs
      chain.rs
      policy.rs
      oracle.rs                              <- typestate admission (extended with L0Transformed)
      merkle.rs
      verify.rs
      proof_builder.rs
      fault.rs
      q16.rs
      errors.rs                              <- L0_SUBTYPE_MISSING added (v0.4-R1)
    tests/
      golden.rs
      invariants_test.rs
      time_test.rs
      ordering_test.rs
      failure_test.rs
      tenant_test.rs
      epistemic_test.rs                      <- v0.4 addition
      l0_golden.rs                           <- v0.4 addition
      l0_commitment_order_test.rs            <- v0.4 addition: SRS-EC-SHALL-009
      l0_mode_test.rs                        <- v0.4 addition: INV-009 epistemic_mode
      l0_subtype_test.rs                     <- v0.4-R1: SUB-RULE-001 through SUB-RULE-004
      l0_containment_equivalence_test.rs     <- v0.4-R1: SRS-EC-SHALL-011
  typescript/
    package.json
    src/
      (mirrors Rust structure, epistemic/ module added including subtype.ts)
    tests/
      golden.test.ts
      epistemic.test.ts                      <- v0.4 addition
      l0_subtype.test.ts                     <- v0.4-R1 addition
  python/
  cli/
    src/main.rs                              <- l0 subcommand (diverge sensitivity/equivalence split; subtype verify added v0.4-R1)
  docs/
    ARCHITECTURE.md
    INVARIANTS.md
    QUICKSTART.md
    GOLDEN-VECTORS.md
    API-REFERENCE.md
    EPISTEMIC.md                             <- v0.4 addition: L0 integration guide
```

---

## 10. Build Sequence (Extended)

### Phase 0 — Golden Vectors (Week 1)

1. Extract canonical records from C99 reference (unchanged)
2. Compute expected commitments (unchanged)
3. Document edge cases (unchanged)
4. **Produce L0 PRNG golden vectors** from `axioma-l0` C99 reference (L0-PV-001 through L0-PV-004)
5. **Produce L0 canonicalisation golden vectors** (L0-CV-001 through L0-CV-007)
6. **Produce L0 evidence record examples** (session, seed, sim, canon, baseline)
7. **Produce L0 failure vectors** (L0_COMMITMENT_ORDER, L0_CONTAINMENT_FAILURE, L0_SUBTYPE_MISSING)
8. **Produce containment equivalence test stub** (`l0_divergence_proof.json` — structure only; values deployment-specific)
9. Publish `golden/vectors.json` and `golden/l0/`
10. Write standalone C99 verifier (extended with L0 verification including subtype checks)

**Gate:** All golden vectors verified including L0 vectors and failure vectors. Nothing else proceeds.

### Phase 1 — Rust SDK (Weeks 2–6)

Weeks 2–5: unchanged from v0.3.

**Week 6 additions (v0.4 + v0.4-R1):**
1. `epistemic/` module — all builders and types
2. `L0Transformed` typestate stage in admission pipeline
3. `epistemic_mode` field in `sdk_context` (INV-009 extension)
4. L0-specific error classes in `errors.rs` (including `L0_SUBTYPE_MISSING`)
5. `epistemic/subtype.rs` — SubtypeVerifier enforcing SUB-RULE-001 through SUB-RULE-004
6. `axioma-l0` FFI bindings (read-only: SDK receives L0 outputs, does not call PRNG)
7. L0 golden vector tests
8. Commitment order enforcement test (SRS-EC-SHALL-009)
9. Subtype verification tests (SUB-RULE-001 through SUB-RULE-004)
10. `epistemic/divergence.rs` split: `TransformationSensitivityTest` (SHALL-008) and `ContainmentEquivalenceTest` (SHALL-011)
11. `axioma verify invariants` extended with L0 and SUB-RULE checks
12. CLI `axioma l0` subcommand: `session inspect`, `seed replay`, `canon verify`, `diverge sensitivity`, `diverge equivalence`, `subtype verify`

**Gate:** All golden vectors pass including L0, failure, and subtype vectors. All INV constraints enforced. SUB-RULE-001 through SUB-RULE-004 enforced. Both divergence test types functional. CLI complete.

### Phase 2 — TypeScript SDK (Weeks 7–10)

Same features including `epistemic` module with `subtype.ts`. Same vectors. Same conformance gate.

**Gate:** Identical commitments to Rust for all test vectors, including L0 failure outputs and `L0_SUBTYPE_MISSING`.

### Phase 3 — Python SDK (Deferred)

PyO3 Rust core. Triggered by market demand.

---

## 11. Consensus Decisions

All v0.3 consensus decisions are unchanged and hold. The following v0.4 decisions are added:

| Question | Decision | Source |
|----------|----------|--------|
| PRNG algorithm | PCG-64 — period 2^64, C99 trivial, no cryptographic requirement | L0-SPEC-001 §3.1 |
| Prompt transformation approach | Structural canonicalisation (total) — not whitelist stripping (conditional) | L0-SPEC-001 §4.1 |
| SRS-EC-SHALL numbering | Flat sequential — SRS-EC-SHALL-009, 010, 011 — no sub-numbering | L0-SPEC-001 §11 |
| L0-SPEC-001 format | Single unified document — math and structures inseparable at L0 | L0-SPEC-001 |
| §1.4 PRNG carve-out | Explicit — SDK records evidence, does not execute PRNG | §1.4 this document |
| L0 evidence tags | `AX:STATE:v1` and `AX:OBS:v1` with `l0_subtype` field — no new top-level tags | §5.1.1 |
| `epistemic_mode` field | Required in all sdk_context, `"EC-D1"` or `"NONE"` | INV-009 extension |
| L0 SDK layer boundary | SDK records L0 evidence; `axioma-l0` C99 AGPL executes L0 logic | §1.4, §1.2, §5.2 |
| L0 admission stage | `L0Transformed` typestate between `Unvalidated` and `Validated` | §5.1.2 |
| l0_subtype assurance gap (v0.4-R1) | SUB-RULE-001 through SUB-RULE-004 mandatory; `L0_SUBTYPE_MISSING` error class; `subtype.rs` module | L0-SPEC-001 §11.1; INV-007 §4.7 |
| SHALL-008 split (v0.4-R1) | SHALL-008 = transformation sensitivity test; SHALL-011 = containment equivalence test; two distinct protocols | L0-SPEC-001 §11, §12.7a, §12.7b |
| Governance adversary class (v0.4-R1) | Deployment must declare and commit adversary class V-A/V-B/V-C; V-A insufficient for any epistemic security claim | L0-SPEC-001 §8.2, §12.5 |
| OQ-003 closed (v0.4-R1) | D_crit_scaled = 13,600 for N=10,000 at α=0.05; derivation documented; constant frozen | L0-SPEC-001 §14 |

---

## 12. Success Criteria

All v0.3 criteria hold. The following are added for v0.4 and v0.4-R1:

- [ ] L0 golden vectors verified against `axioma-l0` C99 reference
- [ ] `epistemic_mode` field present in all `sdk_context` payloads
- [ ] L0_COMMITMENT_ORDER error returned and canonical when session not committed
- [ ] L0_CONTAINMENT_FAILURE error returned and canonical when ε exceeded
- [ ] L0_SUBTYPE_MISSING error returned and canonical for EC-D1 records missing `l0_subtype` (v0.4-R1)
- [ ] SUB-RULE-001 through SUB-RULE-004 enforced in `axioma verify invariants` and `axioma l0 subtype verify` (v0.4-R1)
- [ ] `L0Transformed` typestate stage enforced at compile time (Rust)
- [ ] TypeScript `epistemic` module produces identical L0 evidence records to Rust
- [ ] CLI `axioma l0` subcommand functional: session inspect, seed replay, canon verify, diverge sensitivity, diverge equivalence, subtype verify
- [ ] `axioma verify invariants` includes L0 and SUB-RULE checks
- [ ] BREACH integrity: no test vector converts BREACH to PERMITTED
- [ ] L0 mode records produce different hashes to NONE mode records (INV-001 + INV-009)
- [ ] `axioma l0 diverge sensitivity` and `axioma l0 diverge equivalence` produce distinct proof records committable to L6 (v0.4-R1)
- [ ] Adversary class declared in deployment configuration and committed to L6 for governance probing test (v0.4-R1)

---

## 13. Audit Record

### v0.3 → v0.4

| Finding / Addition | Source | Severity | Resolution |
|---|---|---|---|
| L0 admission pipeline stage missing | L0-SPEC-001 requirement | Critical | `L0Transformed` typestate stage added between `Unvalidated` and `Validated` |
| §1.4 prohibition ambiguous for L0 seed commitment records | Architecture boundary | High | Explicit PRNG carve-out added to §1.4 |
| `sdk_context` does not record epistemic mode | INV-009 gap | High | `epistemic_mode` field added; `"EC-D1"` or `"NONE"` required |
| No SDK error type for L0 commitment order violation | SRS-EC-SHALL-009 | High | `L0_COMMITMENT_ORDER` added to INV-007 failure class table |
| No SDK error type for L0 containment failure | SRS-EC-SHALL-004 | High | `L0_CONTAINMENT_FAILURE` added to INV-007 failure class table |
| New L0 evidence types require tag strategy | DVEC-001 §4.4 | Medium | `l0_subtype` field on existing tags; no new top-level tags; domain separation registry unchanged |
| Golden vector suite has no L0 coverage | Test completeness | Medium | `golden/l0/` directory added with 11 PRNG and canonicalisation vectors |
| INV-004 numeric bounds undeclared for L0 fields | Bounds completeness | Medium | L0 numeric field bounds table added to INV-004 |
| Profile B has no epistemic governance sub-profile | Consumer clarity | Low | Epistemic governance engineer sub-profile added to §2.2 |
| CLI has no L0 inspection commands | Developer experience | Low | `axioma l0` subcommand added |

### v0.4 → v0.4-R1

| Finding / Addition | Source | Severity | Resolution |
|---|---|---|---|
| l0_subtype carries domain-significant meaning without enforcement | H1 — Review A | High | SUB-RULE-001 through SUB-RULE-004 added to L0-SPEC-001 §11.1; `L0_SUBTYPE_MISSING` error class added to INV-007; subtype vocabulary table added; `epistemic/subtype.rs` module; `axioma l0 subtype verify` CLI command |
| SRS-EC-SHALL-008 tests wrong comparison | H2 — Review A | High | SHALL-008 renamed transformation sensitivity test (raw eval vs L0-normalised); SHALL-011 added as containment equivalence test (L0-normalised vs production); two distinct CLI commands and proof record types |
| Preservation Theorem overclaims | M1 — Review A | Medium | Downgraded to Preservation Conformance Objective in L0-SPEC-001 §4.3; SHALL-005 updated; scope explicitly limited to syntactic domain |
| Governance verification adversary too weak | M2 — Review A | Medium | Three adversary classes defined (V-A/V-B/V-C); V-A declared insufficient; deployment must declare and commit adversary class; §12.5 protocol extended |
| Channel coverage proves canonicalisation not leakage closure | M3 — Review A | Medium | §12.6 split into 12.6a (syntactic coverage) and 12.6b (behavioural composition test); SHALL-007 verification updated to reference both |
| Projection may itself become detectable signal | M4 — Review A | Medium | Property 6 (observability smoothing with baseline-calibrated depth jitter on every call) added to L0-SPEC-001 §6.3 |
| "Idempotence is complete correctness check" overclaims | L1 — Review A | Low | Corrected in SHALL-010: idempotence is necessary not sufficient; wording clarified |
| Metadata set tone too strong | L2 — Review A | Low | L0-SPEC-001 §4.5 reframed: role vocabulary closed, automaton carries generality, version-increment discipline explicit |
| OQ-003 non-blocking but touches verification soundness | L4 — Review A | Low | Closed in L0-SPEC-001 §14: D_crit_scaled = 13,600 for N=10,000 at α=0.05; derivation documented; constant frozen |
| Projection padding "ledger records reality" challenge | Review C | Low | Explicit padding invariant added to L0-SPEC-001 §6.3 property 3: synthetic entries are projection artefacts, not ledger artefacts; fully reproducible from committed seed |

### v0.4-R1 → v0.4-R2

| Finding / Addition | Source | Severity | Resolution |
|---|---|---|---|
| Inverse CDF widening gap — `u × H.total` may overflow 32-bit | G1 — Gemini | High | `(uint64_t)u × (uint64_t)H.total` mandated; DVEC-001 §7.2 anchor added; L0-SPEC-001 §5.4 updated |
| Jitter-epsilon property 6 was an assertion not a mechanism | G2 — Gemini | High | `safe_envelope()` intersected-range mechanism specified; PRNG samples only from safe subset; [0,0] on boundary; no looping; L0-SPEC-001 §6.3 property 6 rewritten |
| ε_g must be budget-scoped, not a static constant | G3 — Gemini | High | ε_g(Q) formalised; monotonicity invariant Q₁ < Q₂ → ε_g(Q₁) ≥ ε_g(Q₂); `l0_boundary_config_t` carries `query_budget_Q`; §12.5 V-C protocol updated; L0-SPEC-001 §8.2 rewritten |
| Subtype enforcement provenance-dependent, not payload-deterministic | R1 — Review A | Medium | Three-way layer discriminant adopted: non-null vocabulary-valid = L0, null/absent = non-L0, present-but-empty = malformed; `L0_SUBTYPE_MISSING` scoped to present-but-empty only; SUB-RULE-001 through SUB-RULE-004 rewritten; SDK enforcement text updated |
| §12.6b measured statistic undefined — Mann-Whitney presupposes scalarisation | R2 — Review A | Medium | Statistic pre-declaration mandated; five permitted statistics tabulated; commitment to L6 before test execution; L0-SPEC-001 §12.6b updated |
| Marginal-only fingerprint replay limitation silent | O1 — Review A | Low | Documented as known limitation in L0-SPEC-001 §5.2; registered as OQ-004 in §14; path to bivariate/copula extension specified |

### v0.4-R2 → v0.4-LOCKED

| Finding / Addition | Source | Severity | Resolution |
|---|---|---|---|
| Q budget enforcement delegated to L5 — L0 has no self-defence | G4 — Gemini | High | `L0_BUDGET_EXCEEDED` error class added to INV-007; `L0_SESSION_FLAG_BUDGET_EXCEEDED` flag added; `l0_admit()` gate in §10.2 enforces budget internally; L5 enforcement is defence-in-depth, not the authority |
| PRNG range scaling formula unspecified — modulo bias and float risk | G5 — Gemini | High | `sample_in_range()` formula mandated in §6.3: `Δd_min + (((uint64_t)u × (uint64_t)R) >> 32)`; modulo and float explicitly forbidden; widening required per DVEC-001 §7.2 |

### Full Audit Trail

| Version | Findings Closed | INV Count | Status |
|---------|-----------------|-----------|--------|
| v0.1 | — | 0 | Draft |
| v0.2 | 8 critical (Review A), 3 strategic (Review B) | 7 | System-level closure |
| v0.3 | 7 second-order (Review A), 2 strategic (Review B) | 10 | Production Gold |
| v0.4 | 10 L0 integration items | 10 (unchanged) | Superseded |
| v0.4-R1 | 10 multi-review findings (2 high, 4 medium, 4 low) | 10 (unchanged) | Superseded |
| v0.4-R2 | 6 third-round findings (3 high, 2 medium, 1 low) | 10 (unchanged) | Superseded |
| v0.4-LOCKED | 2 final findings (2 high) | 10 (unchanged) | Production Gold |

---

*AXIOMA-SDK-STRATEGY-001 v0.4-LOCKED — Production Gold*
*William Murray — SpeyTech — April 2026*
*Patent GB2521625.0*
*Source authority for L0: L0-SPEC-001 v1.3-LOCKED · DOI 10.5281/zenodo.19489291*
*Review A sign-off: Production Gold*
*Review B sign-off: Production Gold*
*Review C sign-off: Production Gold*
