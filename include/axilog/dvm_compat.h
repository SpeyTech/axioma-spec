/**
 * @file dvm_compat.h
 * @brief DVM compatibility and platform abstraction
 *
 * DVEC: v1.3
 * DETERMINISM: D1 — Strict Deterministic
 * MEMORY: Zero Dynamic Allocation
 *
 * Copyright (c) 2026 The Murray Family Innovation Trust
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Patent: UK GB2521625.0
 *
 * @traceability SRS-007-SHALL-005, SRS-011-SHALL-001, SRS-011-SHALL-002
 */

#ifndef AXILOG_DVM_COMPAT_H
#define AXILOG_DVM_COMPAT_H

#include <stdint.h>

/**
 * @brief Encode 64-bit unsigned integer as little-endian bytes
 *
 * SRS-007-SHALL-005: Length encoding SHALL be little-endian 64-bit.
 * SRS-011-SHALL-001: Cross-platform identity SHALL be verified.
 *
 * @param value The 64-bit value to encode
 * @param out   Output buffer (must be at least 8 bytes)
 */
static inline void axilog_le64_encode(uint64_t value, uint8_t out[8])
{
    out[0] = (uint8_t)(value & 0xFFU);
    out[1] = (uint8_t)((value >> 8) & 0xFFU);
    out[2] = (uint8_t)((value >> 16) & 0xFFU);
    out[3] = (uint8_t)((value >> 24) & 0xFFU);
    out[4] = (uint8_t)((value >> 32) & 0xFFU);
    out[5] = (uint8_t)((value >> 40) & 0xFFU);
    out[6] = (uint8_t)((value >> 48) & 0xFFU);
    out[7] = (uint8_t)((value >> 56) & 0xFFU);
}

/**
 * @brief Decode little-endian bytes to 64-bit unsigned integer
 *
 * @param in Input buffer (must be at least 8 bytes)
 * @return Decoded 64-bit value
 */
static inline uint64_t axilog_le64_decode(const uint8_t in[8])
{
    return ((uint64_t)in[0]) |
           ((uint64_t)in[1] << 8) |
           ((uint64_t)in[2] << 16) |
           ((uint64_t)in[3] << 24) |
           ((uint64_t)in[4] << 32) |
           ((uint64_t)in[5] << 40) |
           ((uint64_t)in[6] << 48) |
           ((uint64_t)in[7] << 56);
}

/**
 * @brief Encode 32-bit unsigned integer as big-endian bytes
 *
 * Required for SHA-256 message length encoding (FIPS 180-4).
 *
 * @param value The 32-bit value to encode
 * @param out   Output buffer (must be at least 4 bytes)
 */
static inline void axilog_be32_encode(uint32_t value, uint8_t out[4])
{
    out[0] = (uint8_t)((value >> 24) & 0xFFU);
    out[1] = (uint8_t)((value >> 16) & 0xFFU);
    out[2] = (uint8_t)((value >> 8) & 0xFFU);
    out[3] = (uint8_t)(value & 0xFFU);
}

/**
 * @brief Decode big-endian bytes to 32-bit unsigned integer
 *
 * @param in Input buffer (must be at least 4 bytes)
 * @return Decoded 32-bit value
 */
static inline uint32_t axilog_be32_decode(const uint8_t in[4])
{
    return ((uint32_t)in[0] << 24) |
           ((uint32_t)in[1] << 16) |
           ((uint32_t)in[2] << 8) |
           ((uint32_t)in[3]);
}

/**
 * @brief Encode 64-bit unsigned integer as big-endian bytes
 *
 * Required for SHA-256 message length encoding (FIPS 180-4).
 *
 * @param value The 64-bit value to encode
 * @param out   Output buffer (must be at least 8 bytes)
 */
static inline void axilog_be64_encode(uint64_t value, uint8_t out[8])
{
    out[0] = (uint8_t)((value >> 56) & 0xFFU);
    out[1] = (uint8_t)((value >> 48) & 0xFFU);
    out[2] = (uint8_t)((value >> 40) & 0xFFU);
    out[3] = (uint8_t)((value >> 32) & 0xFFU);
    out[4] = (uint8_t)((value >> 24) & 0xFFU);
    out[5] = (uint8_t)((value >> 16) & 0xFFU);
    out[6] = (uint8_t)((value >> 8) & 0xFFU);
    out[7] = (uint8_t)(value & 0xFFU);
}

#endif /* AXILOG_DVM_COMPAT_H */
