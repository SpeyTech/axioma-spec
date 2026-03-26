/**
 * @file commitment.h
 * @brief Domain-separated cryptographic commitment interface
 *
 * DVEC: v1.3
 * DETERMINISM: D1 — Strict Deterministic
 * MEMORY: Zero Dynamic Allocation
 *
 * Copyright (c) 2026 The Murray Family Innovation Trust
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Patent: UK GB2521625.0
 *
 * @traceability SRS-007-SHALL-001, SRS-007-SHALL-002, SRS-007-SHALL-003,
 *               SRS-007-SHALL-004, SRS-007-SHALL-005
 */

#ifndef AXILOG_COMMITMENT_H
#define AXILOG_COMMITMENT_H

#include "types.h"
#include <stdint.h>
#include <stddef.h>

/**
 * @brief Compute domain-separated cryptographic commitment
 *
 * Computes: SHA-256(tag || LE64(payload_len) || payload)
 *
 * SRS-007-SHALL-001: Evidence commitment SHALL use SHA-256.
 * SRS-007-SHALL-002: Commitment function SHALL use domain separation.
 * SRS-007-SHALL-003: Domain separation SHALL use format:
 *                    SHA-256(tag || LE64(len) || payload)
 * SRS-007-SHALL-004: Tag SHALL be ASCII, null-terminated, NOT included
 *                    in payload_len.
 * SRS-007-SHALL-005: Length encoding SHALL be little-endian 64-bit.
 *
 * @param tag         ASCII tag string (null-terminated)
 * @param payload     Payload bytes (RFC 8785 canonicalised if JSON)
 * @param payload_len Byte count of payload (NOT including any null terminator)
 * @param out_commit  Output buffer for 32-byte SHA-256 hash
 * @param faults      Fault context for error propagation
 *
 * @pre tag != NULL
 * @pre payload != NULL || payload_len == 0
 * @pre out_commit != NULL
 * @pre faults != NULL
 *
 * @post On success: out_commit contains valid SHA-256 hash
 * @post On failure: faults->domain == 1, out_commit zeroed
 */
void axilog_commit(
    const char    *tag,
    const uint8_t *payload,
    uint64_t       payload_len,
    uint8_t        out_commit[32],
    ct_fault_flags_t *faults
);

#endif /* AXILOG_COMMITMENT_H */
