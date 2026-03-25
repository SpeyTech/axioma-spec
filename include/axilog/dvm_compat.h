/* DVEC: v1.3 */
/* AXILOG DETERMINISM: D1 — Strict Deterministic */
/* SRS-003-SHALL: All fixed-point arithmetic via dvm_* wrappers */
#ifndef AXILOG_DVM_COMPAT_H
#define AXILOG_DVM_COMPAT_H

#include <stdint.h>
#include "types.h"

/*
 * Fixed-point arithmetic primitives — Q16.16 minimum.
 * Per DVM-SPEC-001 §4 and DVEC-001 v1.3 §7.2 (Widen-Then-Operate).
 *
 * All operations: widen → operate → saturate → record fault.
 * Raw +, -, *, / on fixed-point types are FORBIDDEN (DVEC §12.2).
 */

/* Q16.16 addition with saturation and fault propagation */
int32_t dvm_add_q16(int32_t a, int32_t b, ct_fault_flags_t *faults);

/* Q16.16 subtraction with saturation and fault propagation */
int32_t dvm_sub_q16(int32_t a, int32_t b, ct_fault_flags_t *faults);

/* Q16.16 multiplication with saturation and fault propagation */
int32_t dvm_mul_q16(int32_t a, int32_t b, ct_fault_flags_t *faults);

/* Q16.16 division with saturation and fault propagation */
int32_t dvm_div_q16(int32_t a, int32_t b, ct_fault_flags_t *faults);

#endif /* AXILOG_DVM_COMPAT_H */
