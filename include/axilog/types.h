/**
 * @file types.h
 * @brief Core type definitions for the Axilog substrate
 *
 * DVEC: v1.3
 * DETERMINISM: D1 — Strict Deterministic
 * MEMORY: Zero Dynamic Allocation
 *
 * Copyright (c) 2026 The Murray Family Innovation Trust
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Patent: UK GB2521625.0
 *
 * @traceability SRS-001-SHALL-001, SRS-001-SHALL-002, SRS-005-SHALL-001,
 *               SRS-005-SHALL-004, SRS-005-SHALL-006
 */

#ifndef AXILOG_TYPES_H
#define AXILOG_TYPES_H

#include <stdint.h>
#include <stddef.h>

/**
 * @brief Fault context for explicit fault propagation
 *
 * SRS-005-SHALL-001: All runtime primitives capable of arithmetic, logical,
 *                    state, or ledger failure SHALL accept a fault context.
 * SRS-005-SHALL-004: Runtime fault signalling SHALL NOT use errno.
 * SRS-005-SHALL-006: Fault flags SHALL NOT be cleared implicitly.
 *
 * Memory layout: 8 bytes, no padding required on any platform.
 * NO BITFIELDS — bitfield layout is implementation-defined and breaks
 * cross-platform determinism.
 */
typedef struct {
    uint8_t overflow;      /**< Arithmetic overflow detected */
    uint8_t underflow;     /**< Arithmetic underflow detected */
    uint8_t div_zero;      /**< Division by zero attempted */
    uint8_t saturation;    /**< Invalid saturation operation */
    uint8_t narrowing;     /**< Invalid narrowing conversion */
    uint8_t domain;        /**< Domain validation failure */
    uint8_t ledger_fail;   /**< Ledger operation failure */
    uint8_t _reserved;     /**< Reserved for alignment */
} ct_fault_flags_t;

/**
 * @brief Check if any fault flag is set
 * @param f Pointer to fault context
 * @return Non-zero if any fault is set, zero otherwise
 *
 * SRS-005-SHALL-002: A function detecting a fault SHALL record that fault
 *                    in the supplied fault context before returning.
 */
static inline int ct_fault_any(const ct_fault_flags_t *f)
{
    return (f->overflow | f->underflow | f->div_zero |
            f->saturation | f->narrowing | f->domain | f->ledger_fail);
}

/**
 * @brief Initialize fault context to clean state
 * @param f Pointer to fault context
 *
 * Note: This is the ONLY permitted way to clear faults.
 * Explicit clearing at initialization boundary only.
 */
static inline void ct_fault_init(ct_fault_flags_t *f)
{
    f->overflow = 0;
    f->underflow = 0;
    f->div_zero = 0;
    f->saturation = 0;
    f->narrowing = 0;
    f->domain = 0;
    f->ledger_fail = 0;
    f->_reserved = 0;
}

#endif /* AXILOG_TYPES_H */
