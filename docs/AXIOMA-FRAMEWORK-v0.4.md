# Axioma
## A Verifiable AI Execution Layer for Regulated Industries
### Framework Specification v0.4

**Author:** William Murray, SpeyTech
**Patent:** UK GB2521625.0 (Murray Deterministic Computing Platform)
**Status:** Pre-external-review — final internal audit complete
**Supersedes:** AXIOMA-FRAMEWORK-v0.3
**Date:** March 2026

---

## Revision History

| Version | Status | Notes |
|---------|--------|-------|
| v0.1 | Superseded | Initial conceptual draft |
| v0.2 | Superseded | Determinism Envelope, Evidence Type System, LLM Oracle model, Agent Totality Contract, Replay Contract, certification bifurcation, drift detection, document provenance, commercial reframe |
| v0.3 | Superseded | Dependency Closure, Evidence Completeness Invariant, Oracle Input Completeness, Replay Equivalence Classes, Policy Soundness, Time-as-Oracle, Cost Governance, Q16.16 RRF |
| v0.4 | Current | Ledger Total Order guarantee, Time Oracle monotonicity, Oracle terminology unification, AX:OBS:v1 formal schema (RFC 8785/JCS), C99 commitment implementation |

---

## The One-Line Statement

Axioma is a verifiable AI execution layer that provides cryptographic
proof of AI behaviour across deterministic and non-deterministic
components — from fixed-point computation to LLM oracle calls —
with certification evidence packages for DO-178C, IEC 62304,
ISO 26262, EU AI Act Article 9, and ISO/IEC 42001.

> *"Turing completeness is a vulnerability. Totality is a feature."*
> — DVM-SPEC-001 v1.0-rc1

**The system is deterministic where possible, governed where not,
and provable everywhere.**

---

## 1. The Problem Axioma Solves

Every organisation deploying AI in regulated industries faces
four unsolved problems:

**Problem 1 — The Trust Gap**
AI systems produce outputs. Nobody can prove why they produced
those outputs, whether they will produce the same output tomorrow,
or whether the outputs satisfy the formal requirements of a
regulated environment. "It passed our tests" is not a
certification argument.

**Problem 2 — The Audit Gap**
Regulatory bodies require audit trails. Current AI systems produce
logs. Logs are not audit trails. An audit trail is cryptographically
verifiable, tamper-evident, semantically typed, and independently
reproducible. No mainstream AI framework provides this.

**Problem 3 — The Governance Gap**
AI governance frameworks exist as documents. They define what
should happen. They do not mechanically enforce that it does
happen. The gap between a governance policy and a governance
proof is where regulated industry AI deployments fail.

**Problem 4 — The Certification Gap**
DO-178C, IEC 62304, ISO 26262, and EU AI Act Article 9 require
formal evidence of correctness. Current AI architecture produces
demos. Axioma produces evidence.

---

## 2. What Axioma Is

Axioma is three things simultaneously:

**A Specification**
An open, formally structured framework defining what a verifiable
enterprise AI system must do, prove, and record.
Language-agnostic. Domain-agnostic. Independently implementable.

**A Reference Architecture**
A concrete, layered architecture showing how the specification
is satisfied in practice — from data ingestion through inference,
monitoring, and external verification.

**A Commercial Platform**
A production SDK and toolchain implementing the reference
architecture, commercially licensed for regulated industry
deployment.

```
Open (GPL-3.0):      Axioma Specification + reference implementations
Closed (Commercial): Axioma Platform SDK + libdvm + Governance Engine
```

---

## 3. Oracle Boundary Unification

*Introduced in v0.4 — unifies oracle terminology across the
specification.*

All external interactions in Axioma — LLM inference, time
injection, vector retrieval, sensor inputs — are instances of
the Oracle Boundary defined by DVM-SPEC-001 §6.

> **All oracle interactions (LLM, time, retrieval, sensor) are
> governed by the Oracle Boundary Contract. The Oracle Boundary
> is the single point through which external entropy enters the
> system. Nothing outside this boundary influences system state
> without a corresponding evidence record.**

This yields three named oracle types:

| Oracle Type | DVM Classification | Axioma Layer |
|-------------|-------------------|--------------|
| Oracle Boundary Gateway | External computation oracle | Layer 3 Path B |
| Time Oracle | Temporal input oracle (DVM §6.4) | Layer 5, Layer 6 |
| Knowledge Oracle | Retrieval oracle | Layer 4 |

All three satisfy the Oracle Boundary Contract. All three
produce AX:OBS:v1 evidence records before their outputs
influence system state.

---

## 4. The Determinism Envelope

A naive claim that "Axioma is a deterministic system" is false.
Layer 3 Path B (LLM calls) is observably non-deterministic.
The correct and stronger claim is:

> **Axioma provides full auditability across deterministic and
> non-deterministic components by formally classifying, governing,
> and committing all behaviour — including non-determinism.**

### 4.1 Determinism Classes

| Class | Name | Meaning |
|-------|------|---------|
| D1 | Strict Deterministic | Bit-identical replay guaranteed. |
| D2 | Constrained Deterministic | Deterministic given complete declared dependency set (§4.4). |
| D3 | Observed Non-Deterministic | Non-deterministic, but fully captured and committed. |

### 4.2 Component Classification

| Component | Layer | Class |
|-----------|-------|-------|
| DVM computation | 1 | D1 |
| certifiable-data | 2 | D1 |
| certifiable-training | 2 | D1 |
| certifiable-quant | 2 | D1 |
| certifiable-inference (Path A) | 3 | D1 |
| Oracle Boundary Gateway (Path B) | 3 | D3 |
| Knowledge Oracle (vector retrieval) | 4 | D2 |
| Agent state machines | 5 | D2 |
| Time Oracle inputs | 5, 6 | D1 |
| Audit ledger | 6 | D1 |
| Governance engine | 7 | D2 |

### 4.3 The Envelope Property

> For any Axioma system, the complete execution history is
> auditable and partially replayable. D1 components are
> bit-identically replayable. D3 components are not replayable
> but are fully evidenced. D2 components are replayable given
> the complete declared dependency set.

### 4.4 Dependency Closure (D2 Components)

For any D2 component, the declared dependency set MUST be complete.

**Definition**

```
F : (S, D) → S'
```

Where S is canonical state and D is the declared dependency set.

D MUST include all inputs such that, when held constant,
F is deterministic.

**Minimum Dependency Set Requirements**

| Dependency Type | Examples |
|----------------|---------|
| Model versions | LLM model ID + version hash, embedding model version |
| Index state | Vector index snapshot hash, document corpus hash |
| Tool versions | External tool version identifiers, API versions |
| Configuration | All runtime parameters influencing behaviour |
| Policy versions | Governance rule set version identifiers |

**Conformance Rule:** A D2 component is non-conformant if any
dependency influencing output is omitted from D.

---

## 5. The Evidence Type System

### 5.1 Evidence Ontology

| Type | Tag | Description |
|------|-----|-------------|
| State Commitment | `AX:STATE:v1` | Canonical DVM state hash |
| Transition Commitment | `AX:TRANS:v1` | State change witness with trigger |
| Observation Record | `AX:OBS:v1` | Oracle interaction record |
| Policy Assertion | `AX:POLICY:v1` | Governance rule evaluation |
| Verification Proof | `AX:PROOF:v1` | Replay or conformance proof |

### 5.2 Evidence Commitment Format

All evidence records use the domain-separated commitment format
from DVM-SPEC-001 §7.1, with RFC 8785 (JCS) canonicalisation:

```
commit(evidence) = SHA-256(tag ‖ LE64(|payload|) ‖ payload)
```

Where:
- `tag` is the ASCII evidence type tag from §5.1
- `payload` is the RFC 8785 canonicalised JSON of the evidence record
- `LE64(|payload|)` is the byte length of the canonical payload,
  encoded as a little-endian 64-bit unsigned integer

**RFC 8785 (JCS) is mandatory.** Standard JSON serialisers
produce non-deterministic key ordering and whitespace. Without
JCS canonicalisation, identical evidence records produce
different hashes — breaking the chain. JCS eliminates this
by defining a canonical byte representation for any JSON value.

### 5.3 AX:OBS:v1 Formal Schema

*Introduced in v0.4. The formal JSON schema for Oracle
observation records, satisfying Oracle Input Completeness (§7
Layer 3) and Dependency Closure (§4.4).*

```json
{
  "$schema": "https://json-schema.org/draft/2020-12/schema",
  "$id": "https://axioma.systems/schemas/ax-obs-v1.json",
  "title": "Axioma Observation Record (AX:OBS:v1)",
  "description": "Cryptographic evidence record for oracle interactions.",
  "type": "object",
  "properties": {
    "evidence_type": {
      "type": "string",
      "const": "AX:OBS:v1"
    },
    "event_id": {
      "type": "string",
      "description": "UUIDv7 or deterministic sequence counter."
    },
    "timestamp_ns": {
      "type": "integer",
      "description": "Nanosecond timestamp injected via Time Oracle (DVM §6.4 compliance). Never read from system clock directly."
    },
    "oracle": {
      "type": "object",
      "properties": {
        "type": {
          "type": "string",
          "enum": [
            "LLM_INFERENCE",
            "DOCUMENT_INGESTION",
            "VECTOR_RETRIEVAL",
            "TIME_INJECTION",
            "DRIFT_CHECK"
          ]
        },
        "provider": {
          "type": "string",
          "description": "e.g. azure_openai, gcp_vertex, local"
        }
      },
      "required": ["type"]
    },
    "dependency_set": {
      "type": "object",
      "description": "Satisfies D2 Dependency Closure invariant (§4.4).",
      "properties": {
        "model_identifier": {
          "type": "string",
          "description": "Fully qualified model version. e.g. gpt-4-0125-preview"
        },
        "model_hash": {
          "type": "string",
          "description": "Optional: SHA-256 of model weights where accessible."
        },
        "parameters": {
          "type": "object",
          "properties": {
            "temperature": { "type": "number" },
            "top_p": { "type": "number" },
            "max_tokens": { "type": "integer" }
          },
          "description": "All generation parameters. Defaults are not acceptable."
        }
      },
      "required": ["model_identifier", "parameters"]
    },
    "input_canonical_hash": {
      "type": "string",
      "description": "SHA-256 of the RFC 8785 canonicalised full input (user prompt + system prompt + tool outputs + context)."
    },
    "output_normalised": {
      "type": "string",
      "description": "Semantically meaningful oracle output, stripped of transit artefacts."
    },
    "cost_governance": {
      "type": "object",
      "description": "Committed cost fields — tamper-evident billing evidence.",
      "properties": {
        "input_tokens": { "type": "integer" },
        "output_tokens": { "type": "integer" },
        "latency_ms": {
          "type": "integer",
          "description": "Wall clock latency injected via Time Oracle."
        },
        "estimated_cost_microcents": { "type": "integer" }
      },
      "required": ["input_tokens", "output_tokens", "latency_ms"]
    }
  },
  "required": [
    "evidence_type",
    "event_id",
    "timestamp_ns",
    "oracle",
    "dependency_set",
    "input_canonical_hash",
    "output_normalised"
  ]
}
```

**Why this schema is unassailable:**

- RFC 8785 eliminates hash mismatch from formatting variation
- Domain separation (`AX:OBS:v1` tag) prevents cross-type collisions
  with AX:STATE:v1, AX:TRANS:v1 — even with identical payloads
- Cost fields inside the canonical payload mean any manipulation
  of billing data invalidates the ledger entry
- `timestamp_ns` from Time Oracle ensures DVM §6.4 compliance
- The schema is stateless — the gateway is a deterministic
  pass-through container

### 5.4 C99 Commitment Implementation

*Reference implementation using libdvm primitives.*

```c
#include <stdint.h>
#include <string.h>
#include "dvm_hash.h"

/*
 * Generate AX:OBS:v1 commitment hash.
 *
 * Parameters:
 *   jcs_payload     - RFC 8785 canonicalised JSON, UTF-8 encoded
 *   payload_len     - byte length of jcs_payload
 *   out_hash        - 32-byte output buffer for SHA-256 commitment
 *
 * Implements DVM-SPEC-001 §7.1 domain-separated commitment:
 *   commit = SHA-256(tag || LE64(|payload|) || payload)
 */
void axioma_obs_commit(
    const uint8_t *jcs_payload,
    uint64_t       payload_len,
    uint8_t        out_hash[32]
) {
    dvm_sha256_ctx ctx;
    dvm_sha256_init(&ctx);

    /* Step 1: Hash the tag ("AX:OBS:v1", no null terminator) */
    const char *tag = "AX:OBS:v1";
    dvm_sha256_update(&ctx, (const uint8_t *)tag, 9);

    /* Step 2: Hash the length as little-endian 64-bit */
    uint8_t len_bytes[8];
    for (int i = 0; i < 8; i++) {
        len_bytes[i] = (uint8_t)((payload_len >> (i * 8)) & 0xFF);
    }
    dvm_sha256_update(&ctx, len_bytes, 8);

    /* Step 3: Hash the canonical payload */
    dvm_sha256_update(&ctx, jcs_payload, payload_len);

    /* Step 4: Finalise */
    dvm_sha256_final(&ctx, out_hash);
}
```

The same pattern applies to all evidence types — substitute
the tag string (`AX:STATE:v1`, `AX:TRANS:v1`, `AX:POLICY:v1`,
`AX:PROOF:v1`) for each type.

### 5.5 Evidence Queryability

```sql
-- All LLM oracle calls in a session
SELECT * FROM axioma_ledger
WHERE evidence_type = 'AX:OBS:v1'
  AND payload->>'oracle.type' = 'LLM_INFERENCE';

-- All policy violations in a time window
SELECT * FROM axioma_ledger
WHERE evidence_type = 'AX:POLICY:v1'
  AND payload->>'result' = 'VIOLATION'
  AND committed_at BETWEEN :start AND :end;

-- Tamper check: any cost manipulation
SELECT * FROM axioma_ledger
WHERE evidence_type = 'AX:OBS:v1'
  AND computed_hash != axioma_recompute_hash(payload);
```

### 5.6 Evidence Completeness Invariant

> All externally observable events that influence system state
> MUST produce a corresponding evidence record prior to any
> state transition.

**No state transition may occur without a preceding
committed cause.**

| Layer | Required Evidence |
|-------|-----------------|
| L3 Path B | AX:OBS:v1 per oracle call |
| L4 | AX:OBS:v1 per document ingestion and retrieval |
| L5 | AX:TRANS:v1 per state transition |
| L6 | AX:OBS:v1 per drift check |
| L7 | AX:POLICY:v1 per policy evaluation |

---

## 6. The Certification Map

### 6.1 Track A — Safety-Critical Certification

**D1 components only. LLM API calls explicitly excluded.**

| Standard | Domain |
|----------|--------|
| DO-178C | Aerospace |
| IEC 62304 | Medical devices |
| ISO 26262 | Automotive |

**Evidence:**
- certifiable-* Merkle provenance chains
- certifiable-harness 368-byte golden references
- certifiable-quant certificates (ε₀ = 2⁻¹⁷)
- certifiable-bench correctness-gated performance reports

### 6.2 Track B — Enterprise Governance Certification

**D2/D3 components. LLM API calls governed as oracle interactions.**

| Standard | Domain |
|----------|--------|
| EU AI Act Article 9 | EU regulated AI |
| ISO/IEC 42001 | AI management systems |
| FCA PS22/3 | UK financial services AI |
| MHRA AI/ML guidance | UK medical AI |

**Evidence:**
- Typed audit ledger
- Oracle call records (AX:OBS:v1)
- Drift detection reports (TV/JSD/PSI)
- Proof-carrying policy assertions
- GPG-signed external anchors

### 6.3 The Combined Track

```
Track A (certifiable-*) + Track B (Axioma governance)
= Unified evidence package: safety + governance
```

No other framework produces this combined package.

---

## 7. The Architecture

```
┌──────────────────────────────────────────────────────────────┐
│                      AXIOMA FRAMEWORK                         │
│           Verifiable AI Execution Layer v0.4                  │
├──────┬───────────────────────────────────────────────────────┤
│  L7  │  GOVERNANCE LAYER                     [D2]            │
│      │  Proof-carrying policies, compliance reports,         │
│      │  external anchors, regulatory export                  │
│      │  Evidence: AX:POLICY:v1, AX:PROOF:v1                 │
├──────┼───────────────────────────────────────────────────────┤
│  L6  │  OBSERVABILITY LAYER                  [D1]            │
│      │  Total-ordered hash-chain ledger, typed evidence,     │
│      │  fixed-point drift detection, cost governance         │
│      │  Evidence: All types, strictly ordered chain          │
├──────┼───────────────────────────────────────────────────────┤
│  L5  │  ENTERPRISE INTEGRATION LAYER         [D2]            │
│      │  Total agent state machines, LangGraph orchestration, │
│      │  Time Oracle compliance, RAG pipelines                │
│      │  Evidence: AX:TRANS:v1 per state transition           │
├──────┼───────────────────────────────────────────────────────┤
│  L4  │  KNOWLEDGE LAYER                      [D2]            │
│      │  pgvector primary, document provenance SHA-256,       │
│      │  Q16.16 fixed-point RRF hybrid search                 │
│      │  Evidence: AX:OBS:v1 per document + retrieval         │
├──────┼───────────────────────────────────────────────────────┤
│  L3  │  INFERENCE LAYER                                      │
│      │  Path A: certifiable-inference         [D1]           │
│      │  Path B: Oracle Boundary Gateway       [D3]           │
│      │  Evidence: AX:STATE:v1 (A), AX:OBS:v1 (B)            │
├──────┼───────────────────────────────────────────────────────┤
│  L2  │  DATA LAYER                           [D1]            │
│      │  certifiable-data/training/quant, Merkle provenance   │
│      │  Evidence: AX:STATE:v1, certifiable-* commitments     │
├──────┼───────────────────────────────────────────────────────┤
│  L1  │  COMPUTATION LAYER                    [D1]            │
│      │  DVM-SPEC-001, libdvm (C99), fixed-point arithmetic,  │
│      │  canonical state, commitment chains                   │
│      │  Evidence: AX:STATE:v1, DVM commitment chain          │
└──────┴───────────────────────────────────────────────────────┘
```

---

## 8. Layer Specifications

### Layer 1 — Computation Layer [D1]

**Contract:**
- Fixed-point arithmetic (Q16.16 minimum, per DVM §4.3)
- Canonical state — byte-identical across platforms (DVM §4)
- Total computation — bounded, non-recursive, static (DVM §5)
- Cryptographic commitment chain (DVM §7)

**Existing:** DVM-SPEC-001 v1.0-rc1 complete
**New work:** libdvm C99 SDK (closed commercial)

---

### Layer 2 — Data Layer [D1]

**Contract:**
- B_t = Pipeline(D, seed, epoch, t) — pure function
- θ_T = T^(T)(θ_0, D, seed) — pure function
- ε₀ = 2⁻¹⁷ formally bounded per quantisation layer
- Merkle provenance chain at every stage

**Evidence:**
- certifiable-* Merkle root commitments
- certifiable-harness 368-byte golden references
- certifiable-bench correctness-gated reports

**Existing:** Complete — nine repositories, production gold

---

### Layer 3 — Inference Layer

#### Path A — certifiable-inference [D1] → Track A

**Contract:** Same input → same output. Always. WCET by
construction. No floating-point.

#### Path B — Oracle Boundary Gateway [D3] → Track B

All LLM interactions governed per §3 Oracle Boundary Contract.

**Six requirements:**

1. **Prompt Canonicalisation** — RFC 8785 applied to full input
2. **Oracle Input Completeness** — all influencing inputs included
   (user prompt, system prompt, tool outputs, context, parameters)
3. **Model Version Pinning** — exact version hash committed;
   version drift rejected
4. **Parameter Pinning** — all generation parameters declared;
   defaults not acceptable
5. **Output Normalisation** — transit artefacts stripped
6. **Pre-Consumption Commitment** — AX:OBS:v1 committed
   before output reaches Layer 5

Evidence schema: §5.3 (AX:OBS:v1 formal JSON schema)
Commitment implementation: §5.4 (C99 using libdvm)

---

### Layer 4 — Knowledge Layer [D2]

#### Document Provenance Contract

1. SHA-256 of source document committed before chunking
2. Authorisation record committed alongside source hash
3. Chunking strategy declared and version-controlled
4. Embedding model version pinned and committed

#### Retrieval Determinism Declaration

| Mode | Behaviour |
|------|-----------|
| D-STRICT | Same query → identical results |
| D-BOUNDED | Same query → bounded variance |
| D-ADAPTIVE | Results evolve with index |

#### Vector Database Architecture

**Primary — pgvector (regulated environments)**

Single database footprint. Data residency inside existing
compliance envelope. No additional vendor. One audit surface.
HNSW indexing: sub-10ms P99.

**Q16.16 Fixed-Point RRF Hybrid Search**

The Reciprocal Rank Fusion formula (`1 / (k + rank)`) MUST be
implemented using Q16.16 fixed-point arithmetic from libdvm.

Rationale: native RRF uses floating-point arithmetic, introducing
platform-specific drift. Fixed-point RRF ensures:
- Deterministic scoring — same ranks → same scores always
- Cross-platform identity — no floating-point divergence
- Audit integrity — score committed in AX:OBS:v1 record

```
Query Input
    │
    ├──► pgvector similarity search (semantic)
    ├──► PostgreSQL FTS (keyword)
    └──► Q16.16 RRF via libdvm → deterministic score
              └──► Committed to AX:OBS:v1
              └──► Passed to Layer 5
```

**Secondary — Qdrant** (>10M vectors, self-hosted)
**Avoid for regulated — Pinecone** (external data residency)

---

### Layer 5 — Enterprise Integration Layer [D2]

#### Agent Totality Contract

Every agent MUST satisfy:

1. **Completeness** — every input in every state maps to a
   defined transition
2. **Closure** — no undeclared states reachable
3. **Boundedness** — all paths bounded; max depth declarable
4. **Fault Validity** — faults are valid states, not exceptions;
   silent failure prohibited

**Mirrors DVM §5 applied to agent orchestration.**

#### Time Oracle Compliance (DVM §6.4)

*Direct system clock access from agent execution is prohibited.*

All timestamps in agent evidence records MUST be:
- Read by the Time Oracle (per §3)
- Injected into canonical state as immutable data
- Committed as AX:OBS:v1 (oracle.type: TIME_INJECTION)
  before use in transitions

**Within agent execution, time is immutable state data.**

#### Time Oracle Monotonicity *(v0.4)*

*Closes replay ambiguity and prevents clock rollback attacks.*

Time Oracle outputs MUST be monotonically non-decreasing.
Any time value less than or equal to the previously committed
timestamp is a protocol violation and MUST be rejected.

Where non-monotonic time is genuinely required (e.g. testing,
simulation), this MUST be explicitly declared in the system
configuration and committed at system initialisation.

**Conformance rule:** A system that accepts a timestamp less
than or equal to the prior committed timestamp without an
explicit non-monotonic declaration is non-conformant.

#### Multi-Agent Coordination

- Message-passing only — no shared state
- Every inter-agent message committed (AX:OBS:v1)
- Coordinator satisfies Totality Contract
- No agent acts on uncommitted input

---

### Layer 6 — Observability Layer [D1]

#### Hash Chain Architecture

Linear SHA-256 hash chain:

```
L_0 = SHA-256("AX:LEDGER:v1" ‖ commit(e_0))
L_t = SHA-256("AX:LEDGER:v1" ‖ L_{t-1} ‖ commit(e_t))
```

#### Ledger Total Order Guarantee *(v0.4)*

*Introduced in v0.4. Closes the event ordering ambiguity.*

> **The Axioma ledger defines a strictly total order over all
> committed events. No two evidence records share the same
> position in the chain. For any two events A and B in the
> ledger, exactly one of the following holds: A precedes B,
> or B precedes A.**

This guarantee means:

- Ordering is never ambiguous — auditors cannot claim
  "events may have occurred simultaneously"
- Concurrent access is serialised via the chain head
  lock (SELECT FOR UPDATE on `audit_chain_heads`)
- Replay always produces events in the same order

**Enforcement:** Per-organisation chain head with SELECT FOR
UPDATE (pattern proven in SpeyBooks M6). No two inserts can
share a chain position — the serialisation lock prevents it.

#### Drift Detection

Fixed-point metrics from certifiable-monitor:

| Metric | Implementation |
|--------|---------------|
| Total Variation (TV) | Q0.32 |
| Jensen-Shannon Divergence (JSD) | Q16.16 via 512-entry LUT |
| Population Stability Index (PSI) | Q16.16 with epsilon smoothing |

Results committed as AX:OBS:v1 (D1). Threshold violations
trigger AX:POLICY:v1.

#### Cost Governance

Token counts, compute cycles, and latency for all oracle
interactions committed to AX:OBS:v1 (see §5.3 schema).
Cost fields are inside the canonical payload before hashing —
any manipulation invalidates the ledger entry.

**Effect:** Finance teams can audit AI spend from the same
tamper-evident ledger as everything else. Cost anomalies are
provable events.

---

### Layer 7 — Governance Layer [D2]

#### Proof-Carrying Governance

Policies produce evidenced arguments, not assertions:

```
Policy: "No PII in LLM inputs"
Result: PASS
Evidence: {
  rule: "PII_SCAN_v1",
  evaluated_records: [AX:OBS:v1:a3f2..., AX:OBS:v1:b7c4...],
  scan_results: [CLEAN, CLEAN],
  scan_model_version: "pii-detector-v2.1.3",
  commitment: AX:POLICY:v1:9e1d...
}
```

#### Policy Soundness Requirement

Policies MUST satisfy:

1. **Deterministic Evaluation** — identical inputs → identical output
2. **Evidence Closure** — references only committed evidence records
3. **Independent Verifiability** — third party can recompute

```
P : E → R where ∀ E : P(E) = deterministic
```

Policies are deterministic programs over committed evidence.
Governance is compiled into the execution model.

#### Compliance Report Generation

| Report | Standard |
|--------|----------|
| EU AI Act Article 9 | EU AI Act |
| ISO 42001 | AI management |
| FCA Model Risk | PS22/3 |
| Safety Evidence Package | DO-178C / IEC 62304 / ISO 26262 |

#### External Anchor Publication

- SHA-256 of all chain heads, all tenants
- GPG-signed commit, public key in repository README
- Append-only transparency log
- Daily cron, 01:00 UTC

Pattern: DESIGN-M7-DETERMINISTIC-INFRASTRUCTURE.

#### certifiable-harness Golden References

368-byte golden reference published in Track A evidence package.
Proves the deployed model is byte-identical to the certified model.

---

## 9. The Replay Contract

### 9.1 Full Replay Set (D1)

1. Initial canonical state S_0
2. Program definition (agent state machines, policy versions)
3. DVM version (libdvm version hash)
4. certifiable-* configuration

### 9.2 Governed Replay Set (D2/D3)

Items 1–4 plus:
5. All oracle inputs from AX:OBS:v1 records
6. Model versions from oracle records
7. Embedding model version from document provenance
8. Tool state declarations from agent records
9. Time Oracle inputs from TIME_INJECTION records

### 9.3 Replay Equivalence Classes

| Level | Name | Formal | Applies to |
|-------|------|--------|-----------|
| L1 | Bit-Equivalent | C' = C | D1 components |
| L2 | State-Equivalent | F(S,D) = F'(S,D) | D2 components |
| L3 | Decision-Equivalent | F(S, O_recorded) → identical decisions | D3 components |

Each system MUST declare the equivalence level per component.

### 9.4 Replay Verification Protocol

```
1. verify-ledger-replay   → D1 commitment chain
2. verify-audit-chain     → Layer 6 total order continuity
3. verify-oracle-records  → D3 oracle inputs complete
4. verify-anchor          → state matches published anchor
5. verify-golden          → deployed model matches certified
```

All five must pass before a restored system is verified.

---

## 10. Commercial Model

### 10.1 Axioma Oracle Gateway — Standalone Entry Product

A deterministic, auditable wrapper for enterprise LLM API calls
producing EU AI Act Article 9 compliance evidence.

**MVP:**
- Azure OpenAI integration
- RFC 8785 prompt canonicalisation
- Model version pinning and drift rejection
- AX:OBS:v1 evidence records with cost fields
- EU AI Act Article 9 compliance export
- Docker container

**Why now:** Addresses EU AI Act compliance for LLM usage
without requiring the full seven-layer platform. Entry point
into the full Axioma relationship.

### 10.2 Full Commercial Stack

```
OPEN (GPL-3.0)
  axioma-spec, axioma-agent, axioma-knowledge,
  axioma-audit, axioma-verify, certifiable-*

COMMERCIAL
  Axioma Oracle Gateway (standalone entry)
  Axioma Platform SDK (full stack)
  axioma-governance (policy engine + reports)
  libdvm (patented C99 SDK)
  Certification evidence tooling
  Enterprise support + SLA
```

### 10.3 Positioning

**Axioma is a verifiable AI execution layer.**
Not a governance framework. Not an observability tool.
Execution infrastructure with governance built in.

| Framework | Scope |
|-----------|-------|
| LangChain / LangGraph | Orchestration only |
| MLflow | Tracking only |
| Weights & Biases | Observability only |
| Pinecone stack | Retrieval only |
| **Axioma** | **Execution + evidence + governance + certification** |

No direct competitor exists in this form.

---

## 11. Build Roadmap

### Phase 0 — Foundation (Months 1–2)
- `axioma-spec` published
- `axioma-knowledge` — Layer 4 with document provenance and Q16.16 RRF
- `axioma-agent` — Layer 5 with Totality Contract and Time Oracle
- `axioma-audit` — Layer 6 with total order guarantee and typed evidence
- **Axioma Oracle Gateway MVP** — standalone entry product
- Demo: SpeyTech knowledge base, full audit trail

### Phase 1 — Observability & Drift (Months 3–4)
- certifiable-monitor drift detection in Layer 6
- Semantic query interface
- External anchor publication
- `axioma-verify` — replay and chain verification

### Phase 2 — Governance Engine (Months 5–7)
- `axioma-governance` — proof-carrying policy engine
- EU AI Act, ISO 42001, FCA PS22/3 report generators
- Operational governance dashboard

### Phase 3 — certifiable-* Integration (Months 8–10)
- Layer 2→3 commitment handoff
- Combined Track A + Track B evidence package
- Golden references in Layer 7

### Phase 4 — libdvm & Platform SDK (Months 11–12)
- libdvm C99 reference implementation
- Axioma Platform SDK
- Domain launch (axioma.systems / axioma.io)
- Licensing framework

---

## Appendix A — Audit Record

### v0.3 → v0.4

| Finding | Source | Resolution |
|---------|--------|------------|
| Ledger Total Order not stated | GPT | §8 L6 |
| Time Oracle monotonicity missing | GPT | §8 L5 |
| Oracle terminology unified | GPT | §3 |
| AX:OBS:v1 formal schema | GPT | §5.3 |
| RFC 8785 JCS requirement | GPT | §5.2 |
| C99 commitment implementation | GPT | §5.4 |

---

## Appendix B — Invariants Summary

| # | Invariant | Section |
|---|-----------|---------|
| 1 | Oracle Boundary unification — all oracles governed by DVM §6 | §3 |
| 2 | Determinism Envelope defined, all components classified | §4 |
| 3 | D2 Dependency Closure satisfied | §4.4 |
| 4 | All evidence typed per ontology | §5.1 |
| 5 | RFC 8785 JCS applied to all evidence payloads | §5.2 |
| 6 | AX:OBS:v1 schema conforms to §5.3 | §5.3 |
| 7 | Evidence Completeness Invariant enforced | §5.6 |
| 8 | Certification track declared per system | §6 |
| 9 | Layer 3 Path B: six oracle requirements satisfied | §8 L3 |
| 10 | Oracle Input Completeness satisfied | §8 L3 |
| 11 | Layer 4: document provenance committed before chunking | §8 L4 |
| 12 | Layer 4: RRF implemented in Q16.16 fixed-point | §8 L4 |
| 13 | Layer 5: Agent Totality Contract satisfied | §8 L5 |
| 14 | Layer 5: Time Oracle compliance (DVM §6.4) | §8 L5 |
| 15 | Layer 5: Time Oracle monotonicity enforced | §8 L5 |
| 16 | Layer 6: Ledger Total Order guaranteed | §8 L6 |
| 17 | Layer 6: hash chain D1, typed, drift detection | §8 L6 |
| 18 | Layer 6: cost fields committed in AX:OBS:v1 | §8 L6 |
| 19 | Layer 7: proof-carrying assertions with citations | §8 L7 |
| 20 | Layer 7: Policy Soundness Requirement satisfied | §8 L7 |
| 21 | Replay equivalence class declared per component | §9.3 |
| 22 | External anchor published and GPG-signed | §8 L7 |

---

## Appendix C — Relationship to SpeyTech Ecosystem

| Axioma Component | SpeyTech Precedent | Status |
|-----------------|-------------------|--------|
| Layer 1 — DVM computation | DVM-SPEC-001 v1.0-rc1 | Complete |
| Layer 1 — libdvm SDK | Planned C99 implementation | In design |
| Layer 2 — Data pipeline | certifiable-data | Production gold |
| Layer 2 — Training | certifiable-training | Production gold |
| Layer 2 — Quantisation | certifiable-quant | Production gold |
| Layer 2→3 handoff | certifiable-deploy | Production gold |
| Layer 3 Path A — Inference | certifiable-inference | Production gold |
| Layer 6 — Hash chain | SpeyBooks M6 audit trail | Production gold |
| Layer 6 — Drift detection | certifiable-monitor | Production gold |
| Layer 6 — Chain verification | certifiable-verify | Production gold |
| Layer 7 — External anchoring | SpeyBooks M7 pattern | Production gold |
| Layer 7 — Golden references | certifiable-harness | Production gold |
| Layer 7 — Performance evidence | certifiable-bench | Production gold |

**11,840+ test assertions. Nine production repositories.
One patent. One published specification. No vapourware.**

---

*v0.4 — Pre-external-review. Final internal audit complete.*
*Deterministic where possible. Governed where not.
Provable everywhere.*

*William Murray · SpeyTech · March 2026*
*Patent GB2521625.0*
