/* DVEC: v1.3 */
/* AXILOG DETERMINISM: D1 — Strict Deterministic */
/* SRS-002-SHALL: All commitments use domain-separated SHA-256 */
#ifndef AXILOG_COMMITMENT_H
#define AXILOG_COMMITMENT_H

#include <stdint.h>
#include "types.h"

/*
 * Domain-separated commitment function.
 * commit(e) = SHA-256(tag || LE64(|payload|) || payload)
 * Per DVM-SPEC-001 §7.1 and Axioma v0.4 §4.3
 */
void axilog_commit(
    const char       *tag,
    const uint8_t    *payload,
    uint64_t          payload_len,
    uint8_t           out_hash[32],
    ct_fault_flags_t *faults
);

#endif /* AXILOG_COMMITMENT_H */
