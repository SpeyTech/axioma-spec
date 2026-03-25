/* DVEC: v1.3 */
/* AXILOG DETERMINISM: D1 — Strict Deterministic */
/* SRS-001-SHALL: All fault conditions represented as typed flags */
#ifndef AXILOG_TYPES_H
#define AXILOG_TYPES_H

#include <stdint.h>

/* Fault flag vector — per DVM-SPEC-001 §4.5 */
typedef struct {
    uint8_t overflow        : 1;
    uint8_t division_by_zero: 1;
    uint8_t invalid_domain  : 1;
    uint8_t saturation      : 1;
    uint8_t reserved        : 4;
} ct_fault_flags_t;

/* Evidence type tags — per Axioma v0.4 §4.4 */
#define AX_TAG_STATE    "AX:STATE:v1"
#define AX_TAG_TRANS    "AX:TRANS:v1"
#define AX_TAG_OBS      "AX:OBS:v1"
#define AX_TAG_POLICY   "AX:POLICY:v1"
#define AX_TAG_PROOF    "AX:PROOF:v1"

/* Chain tags — NOT evidence types, per §4.4 */
#define AX_TAG_LEDGER   "AX:LEDGER:v1"

#endif /* AXILOG_TYPES_H */
