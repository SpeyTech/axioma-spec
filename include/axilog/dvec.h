/**
 * @file dvec.h
 * @brief DVEC v1.3 contract declarations and domain tag registry
 *
 * DVEC: v1.3
 * DETERMINISM: D1 — Strict Deterministic
 * MEMORY: Zero Dynamic Allocation
 *
 * Copyright (c) 2026 The Murray Family Innovation Trust
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Patent: UK GB2521625.0
 *
 * @traceability SRS-001-SHALL-001, SRS-006-SHALL-003, SRS-007-SHALL-006
 */

#ifndef AXILOG_DVEC_H
#define AXILOG_DVEC_H

/**
 * @defgroup DomainTags Domain Separation Registry (DVEC-001 v1.3 §4.4)
 *
 * Evidence type tags — typed evidence records committed to the ledger.
 * Chain tags — ledger protocol prefixes, NOT evidence types.
 *
 * Rules:
 * - Evidence type tags MAY appear as payloads in the ledger chain
 * - Chain tags MAY NOT be used as evidence type identifiers
 * - No tag from either namespace may be reused in the other
 * - No ad hoc tags permitted outside this registry
 *
 * @{
 */

/** @name Evidence Type Tags */
/**@{*/
#define AX_TAG_STATE   "AX:STATE:v1"   /**< Canonical DVM state hash */
#define AX_TAG_TRANS   "AX:TRANS:v1"   /**< State change witness */
#define AX_TAG_OBS     "AX:OBS:v1"     /**< Oracle interaction record */
#define AX_TAG_POLICY  "AX:POLICY:v1"  /**< Governance rule evaluation */
#define AX_TAG_PROOF   "AX:PROOF:v1"   /**< Replay or conformance proof */
/**@}*/

/** @name Chain Tags (Protocol Prefixes) */
/**@{*/
#define AX_CHAIN_LEDGER  "AX:LEDGER:v1"   /**< Axioma hash chain */
#define DVM_CHAIN_LEDGER "DVM:LEDGER:v1"  /**< DVM computation chain */
#define DVM_CHAIN_STATE  "DVM:STATE:v1"   /**< DVM state commitment */
#define DVM_CHAIN_INGRESS "DVM:INGRESS:v1" /**< DVM ingress oracle */
/**@}*/

/** @} */ /* end DomainTags */

/**
 * @brief Determinism class declarations (DVEC-001 v1.3 §1.4)
 *
 * D1 — Strict Deterministic: Bit-identical across all platforms
 * D2 — Constrained Deterministic: Deterministic given declared dependencies
 * D3 — Observed Non-Deterministic: Routed through Oracle Boundary Contract
 */
#define DVEC_CLASS_D1  1  /**< Strict Deterministic */
#define DVEC_CLASS_D2  2  /**< Constrained Deterministic */
#define DVEC_CLASS_D3  3  /**< Observed Non-Deterministic */

/**
 * @brief DVEC version for module headers
 *
 * SRS-001-SHALL-001: Every Axilog module SHALL declare its governing
 *                    DVEC version in the module header.
 */
#define DVEC_VERSION "v1.3"

/**
 * @brief Module determinism class macro
 *
 * SRS-001-SHALL-002: Every Axilog module SHALL declare its determinism
 *                    class as D1, D2, or D3 in its primary header.
 */
#define AXILOG_DETERMINISM_D1 "D1 — Strict Deterministic"
#define AXILOG_DETERMINISM_D2 "D2 — Constrained Deterministic"
#define AXILOG_DETERMINISM_D3 "D3 — Observed Non-Deterministic"

#endif /* AXILOG_DVEC_H */
