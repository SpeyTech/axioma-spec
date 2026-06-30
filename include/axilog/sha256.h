/**
 * @file sha256.h
 * @brief Plain SHA-256 digest interface
 *
 * DVEC: v1.3
 * DETERMINISM: D1 — Strict Deterministic
 * MEMORY: Zero Dynamic Allocation
 *
 * Copyright (c) 2026 Spey Systems LTD
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Patent: UK GB2521625.0
 *
 * Use axilog_commit() for domain-separated evidence records.
 * Use axilog_sha256() only for plain digests (e.g. raw prompt ingress hashing).
 */

#ifndef AXILOG_SHA256_H
#define AXILOG_SHA256_H

#include <stdint.h>
#include <stddef.h>

/**
 * @brief Compute plain SHA-256 digest of data.
 *
 * Not domain-separated. For evidence records use axilog_commit().
 * For ingress hashing (raw prompt hash in l0_admit step 3b), use this.
 *
 * @param out  Output buffer for 32-byte SHA-256 digest
 * @param data Input bytes
 * @param len  Byte count of input
 */
void axilog_sha256(uint8_t out[32], const uint8_t *data, size_t len);

#endif /* AXILOG_SHA256_H */
