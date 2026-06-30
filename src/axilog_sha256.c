// SPDX-License-Identifier: GPL-3.0-or-later
// DVEC: v1.3
// DETERMINISM: D1 — Strict Deterministic
// Copyright (c) 2026 Spey Systems LTD. All rights reserved.
// Patent: UK GB2521625.0

/*
 * axilog_sha256.c — Variable-length SHA-256 and axilog_commit implementation.
 *
 * Implements:
 *   axilog_sha256() from axilog/sha256.h — plain SHA-256 digest
 *   axilog_commit() from axilog/commitment.h — domain-separated commitment
 *
 * SHA-256 constants: FIPS 180-4.
 * No dynamic allocation. No float. No external libraries.
 * All iteration is forward-index order.
 *
 * DVEC invariant — SHA-256 constant identity:
 * The K round constants and initial hash values here MUST be byte-identical
 * to those in sha256_56() in axioma-l0/src/l0_session.c. Both are derived
 * from FIPS 180-4. Any divergence is a conformance failure that will produce
 * different hash outputs on the same input, breaking cross-platform bit-identity
 * between the seed derivation path and the general hashing path.
 * Verification: test_conformance.c asserts sha256_56(56-byte-input) ==
 * axilog_sha256(same-56-byte-input) on a known test vector.
 */

#include "axilog/sha256.h"
#include "axilog/commitment.h"
#include <string.h>
#include <stdint.h>

/* ─────────────────────────────────────────────────────────────────────────── */
/* SHA-256 constants (FIPS 180-4) — identical to l0_session.c sha256_56()     */
/* ─────────────────────────────────────────────────────────────────────────── */

static const uint32_t sha256_k[64] = {
    0x428a2f98u, 0x71374491u, 0xb5c0fbcfu, 0xe9b5dba5u,
    0x3956c25bu, 0x59f111f1u, 0x923f82a4u, 0xab1c5ed5u,
    0xd807aa98u, 0x12835b01u, 0x243185beu, 0x550c7dc3u,
    0x72be5d74u, 0x80deb1feu, 0x9bdc06a7u, 0xc19bf174u,
    0xe49b69c1u, 0xefbe4786u, 0x0fc19dc6u, 0x240ca1ccu,
    0x2de92c6fu, 0x4a7484aau, 0x5cb0a9dcu, 0x76f988dau,
    0x983e5152u, 0xa831c66du, 0xb00327c8u, 0xbf597fc7u,
    0xc6e00bf3u, 0xd5a79147u, 0x06ca6351u, 0x14292967u,
    0x27b70a85u, 0x2e1b2138u, 0x4d2c6dfcu, 0x53380d13u,
    0x650a7354u, 0x766a0abbu, 0x81c2c92eu, 0x92722c85u,
    0xa2bfe8a1u, 0xa81a664bu, 0xc24b8b70u, 0xc76c51a3u,
    0xd192e819u, 0xd6990624u, 0xf40e3585u, 0x106aa070u,
    0x19a4c116u, 0x1e376c08u, 0x2748774cu, 0x34b0bcb5u,
    0x391c0cb3u, 0x4ed8aa4au, 0x5b9cca4fu, 0x682e6ff3u,
    0x748f82eeu, 0x78a5636fu, 0x84c87814u, 0x8cc70208u,
    0x90befffau, 0xa4506cebu, 0xbef9a3f7u, 0xc67178f2u
};

static const uint32_t sha256_h0[8] = {
    0x6a09e667u, 0xbb67ae85u, 0x3c6ef372u, 0xa54ff53au,
    0x510e527fu, 0x9b05688cu, 0x1f83d9abu, 0x5be0cd19u
};

/* ─────────────────────────────────────────────────────────────────────────── */
/* SHA-256 internal context                                                     */
/* ─────────────────────────────────────────────────────────────────────────── */

typedef struct {
    uint32_t state[8];
    uint64_t bit_len;
    uint8_t  buf[64];
    uint32_t buf_len;
} sha256_ctx_t;

static uint32_t rotr32(uint32_t x, uint32_t n)
{
    return (x >> n) | (x << (32u - n));
}

static uint32_t be32_load(const uint8_t *p)
{
    return ((uint32_t)p[0] << 24u) | ((uint32_t)p[1] << 16u) |
           ((uint32_t)p[2] <<  8u) |  (uint32_t)p[3];
}

static void be32_store(uint8_t *p, uint32_t v)
{
    p[0] = (uint8_t)(v >> 24u);
    p[1] = (uint8_t)(v >> 16u);
    p[2] = (uint8_t)(v >>  8u);
    p[3] = (uint8_t)(v);
}

static void sha256_compress(uint32_t state[8], const uint8_t block[64])
{
    uint32_t w[64];
    uint32_t a, b, c, d, e, f, g, h;
    uint32_t i;

    for (i = 0u; i < 16u; i++) w[i] = be32_load(block + i * 4u);
    for (i = 16u; i < 64u; i++) {
        uint32_t s0 = rotr32(w[i-15u],  7u) ^ rotr32(w[i-15u], 18u) ^ (w[i-15u] >>  3u);
        uint32_t s1 = rotr32(w[i- 2u], 17u) ^ rotr32(w[i- 2u], 19u) ^ (w[i- 2u] >> 10u);
        w[i] = w[i-16u] + s0 + w[i-7u] + s1;
    }

    a = state[0]; b = state[1]; c = state[2]; d = state[3];
    e = state[4]; f = state[5]; g = state[6]; h = state[7];

    for (i = 0u; i < 64u; i++) {
        uint32_t S1   = rotr32(e, 6u) ^ rotr32(e, 11u) ^ rotr32(e, 25u);
        uint32_t ch   = (e & f) ^ (~e & g);
        uint32_t tmp1 = h + S1 + ch + sha256_k[i] + w[i];
        uint32_t S0   = rotr32(a, 2u) ^ rotr32(a, 13u) ^ rotr32(a, 22u);
        uint32_t maj  = (a & b) ^ (a & c) ^ (b & c);
        uint32_t tmp2 = S0 + maj;
        h = g; g = f; f = e; e = d + tmp1;
        d = c; c = b; b = a; a = tmp1 + tmp2;
    }

    state[0] += a; state[1] += b; state[2] += c; state[3] += d;
    state[4] += e; state[5] += f; state[6] += g; state[7] += h;
}

static void sha256_init(sha256_ctx_t *ctx)
{
    uint32_t i;
    for (i = 0u; i < 8u; i++) ctx->state[i] = sha256_h0[i];
    ctx->bit_len = 0u;
    ctx->buf_len = 0u;
}

static void sha256_update(sha256_ctx_t *ctx, const uint8_t *data, size_t len)
{
    size_t i;
    for (i = 0u; i < len; i++) {
        ctx->buf[ctx->buf_len] = data[i];
        ctx->buf_len++;
        if (ctx->buf_len == 64u) {
            sha256_compress(ctx->state, ctx->buf);
            ctx->bit_len += 512u;
            ctx->buf_len  = 0u;
        }
    }
}

static void sha256_final(sha256_ctx_t *ctx, uint8_t out[32])
{
    uint32_t i;
    uint64_t total_bits;

    ctx->bit_len += (uint64_t)ctx->buf_len * 8u;
    total_bits = ctx->bit_len;

    ctx->buf[ctx->buf_len] = 0x80u;
    ctx->buf_len++;

    if (ctx->buf_len > 56u) {
        while (ctx->buf_len < 64u) ctx->buf[ctx->buf_len++] = 0u;
        sha256_compress(ctx->state, ctx->buf);
        ctx->buf_len = 0u;
    }
    while (ctx->buf_len < 56u) ctx->buf[ctx->buf_len++] = 0u;

    ctx->buf[56] = (uint8_t)(total_bits >> 56u);
    ctx->buf[57] = (uint8_t)(total_bits >> 48u);
    ctx->buf[58] = (uint8_t)(total_bits >> 40u);
    ctx->buf[59] = (uint8_t)(total_bits >> 32u);
    ctx->buf[60] = (uint8_t)(total_bits >> 24u);
    ctx->buf[61] = (uint8_t)(total_bits >> 16u);
    ctx->buf[62] = (uint8_t)(total_bits >>  8u);
    ctx->buf[63] = (uint8_t)(total_bits);
    sha256_compress(ctx->state, ctx->buf);

    for (i = 0u; i < 8u; i++) be32_store(out + i * 4u, ctx->state[i]);
}

/* ─────────────────────────────────────────────────────────────────────────── */
/* Public API                                                                   */
/* ─────────────────────────────────────────────────────────────────────────── */

void axilog_sha256(uint8_t out[32], const uint8_t *data, size_t len)
{
    sha256_ctx_t ctx;
    sha256_init(&ctx);
    sha256_update(&ctx, data, len);
    sha256_final(&ctx, out);
}

void axilog_commit(const char    *tag,
                   const uint8_t *payload,
                   uint64_t       payload_len,
                   uint8_t        out_commit[32],
                   ct_fault_flags_t *faults)
{
    sha256_ctx_t ctx;
    uint8_t      len_buf[8];

    if (tag == NULL || out_commit == NULL || faults == NULL) {
        if (faults) faults->domain = 1;
        return;
    }
    if (payload == NULL && payload_len > 0u) {
        faults->domain = 1;
        memset(out_commit, 0, 32u);
        return;
    }

    len_buf[0] = (uint8_t)(payload_len);
    len_buf[1] = (uint8_t)(payload_len >>  8u);
    len_buf[2] = (uint8_t)(payload_len >> 16u);
    len_buf[3] = (uint8_t)(payload_len >> 24u);
    len_buf[4] = (uint8_t)(payload_len >> 32u);
    len_buf[5] = (uint8_t)(payload_len >> 40u);
    len_buf[6] = (uint8_t)(payload_len >> 48u);
    len_buf[7] = (uint8_t)(payload_len >> 56u);

    sha256_init(&ctx);
    sha256_update(&ctx, (const uint8_t *)tag, strlen(tag));
    sha256_update(&ctx, len_buf, 8u);
    if (payload_len > 0u) {
        sha256_update(&ctx, payload, (size_t)payload_len);
    }
    sha256_final(&ctx, out_commit);
}
