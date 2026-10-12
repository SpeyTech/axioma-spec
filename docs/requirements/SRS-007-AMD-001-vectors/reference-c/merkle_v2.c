/* merkle_v2.c: clean-room implementation of SRS-007-AMD-001 v0.3 DRAFT
 * (AX:MERKLE:v2 and Appendix A AX:MERKLE:v1) plus the v0.3 vector harness.
 * C99, standard library only. */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ------------------------------------------------------------------ */
/* FIPS 180-4 SHA-256                                                   */
/* ------------------------------------------------------------------ */
typedef struct {
    uint32_t h[8];
    uint64_t total;
    uint8_t buf[64];
    size_t blen;
} sha256_ctx;

static const uint32_t K256[64] = {
    0x428a2f98u,0x71374491u,0xb5c0fbcfu,0xe9b5dba5u,0x3956c25bu,0x59f111f1u,0x923f82a4u,0xab1c5ed5u,
    0xd807aa98u,0x12835b01u,0x243185beu,0x550c7dc3u,0x72be5d74u,0x80deb1feu,0x9bdc06a7u,0xc19bf174u,
    0xe49b69c1u,0xefbe4786u,0x0fc19dc6u,0x240ca1ccu,0x2de92c6fu,0x4a7484aau,0x5cb0a9dcu,0x76f988dau,
    0x983e5152u,0xa831c66du,0xb00327c8u,0xbf597fc7u,0xc6e00bf3u,0xd5a79147u,0x06ca6351u,0x14292967u,
    0x27b70a85u,0x2e1b2138u,0x4d2c6dfcu,0x53380d13u,0x650a7354u,0x766a0abbu,0x81c2c92eu,0x92722c85u,
    0xa2bfe8a1u,0xa81a664bu,0xc24b8b70u,0xc76c51a3u,0xd192e819u,0xd6990624u,0xf40e3585u,0x106aa070u,
    0x19a4c116u,0x1e376c08u,0x2748774cu,0x34b0bcb5u,0x391c0cb3u,0x4ed8aa4au,0x5b9cca4fu,0x682e6ff3u,
    0x748f82eeu,0x78a5636fu,0x84c87814u,0x8cc70208u,0x90befffau,0xa4506cebu,0xbef9a3f7u,0xc67178f2u
};

#define ROTR(x,n) (((x) >> (n)) | ((x) << (32 - (n))))

static void sha256_block(uint32_t h[8], const uint8_t *p)
{
    uint32_t w[64], a, b, c, d, e, f, g, hh, t1, t2;
    int i;
    for (i = 0; i < 16; i++)
        w[i] = ((uint32_t)p[4*i] << 24) | ((uint32_t)p[4*i+1] << 16) |
               ((uint32_t)p[4*i+2] << 8) | (uint32_t)p[4*i+3];
    for (i = 16; i < 64; i++) {
        uint32_t s0 = ROTR(w[i-15],7) ^ ROTR(w[i-15],18) ^ (w[i-15] >> 3);
        uint32_t s1 = ROTR(w[i-2],17) ^ ROTR(w[i-2],19) ^ (w[i-2] >> 10);
        w[i] = w[i-16] + s0 + w[i-7] + s1;
    }
    a=h[0]; b=h[1]; c=h[2]; d=h[3]; e=h[4]; f=h[5]; g=h[6]; hh=h[7];
    for (i = 0; i < 64; i++) {
        uint32_t S1 = ROTR(e,6) ^ ROTR(e,11) ^ ROTR(e,25);
        uint32_t ch = (e & f) ^ (~e & g);
        uint32_t S0 = ROTR(a,2) ^ ROTR(a,13) ^ ROTR(a,22);
        uint32_t mj = (a & b) ^ (a & c) ^ (b & c);
        t1 = hh + S1 + ch + K256[i] + w[i];
        t2 = S0 + mj;
        hh = g; g = f; f = e; e = d + t1; d = c; c = b; b = a; a = t1 + t2;
    }
    h[0]+=a; h[1]+=b; h[2]+=c; h[3]+=d; h[4]+=e; h[5]+=f; h[6]+=g; h[7]+=hh;
}

static void sha256_init(sha256_ctx *c)
{
    static const uint32_t iv[8] = {
        0x6a09e667u,0xbb67ae85u,0x3c6ef372u,0xa54ff53au,
        0x510e527fu,0x9b05688cu,0x1f83d9abu,0x5be0cd19u };
    memcpy(c->h, iv, sizeof iv);
    c->total = 0;
    c->blen = 0;
}

static void sha256_update(sha256_ctx *c, const uint8_t *p, size_t n)
{
    c->total += n;
    while (n > 0) {
        size_t take = 64 - c->blen;
        if (take > n) take = n;
        memcpy(c->buf + c->blen, p, take);
        c->blen += take; p += take; n -= take;
        if (c->blen == 64) { sha256_block(c->h, c->buf); c->blen = 0; }
    }
}

static void sha256_final(sha256_ctx *c, uint8_t out[32])
{
    uint64_t bits = c->total * 8u;
    uint8_t pad = 0x80, zero = 0, len[8];
    int i;
    sha256_update(c, &pad, 1);
    while (c->blen != 56) sha256_update(c, &zero, 1);
    for (i = 0; i < 8; i++) len[i] = (uint8_t)(bits >> (56 - 8*i));
    sha256_update(c, len, 8);
    for (i = 0; i < 8; i++) {
        out[4*i]   = (uint8_t)(c->h[i] >> 24);
        out[4*i+1] = (uint8_t)(c->h[i] >> 16);
        out[4*i+2] = (uint8_t)(c->h[i] >> 8);
        out[4*i+3] = (uint8_t)(c->h[i]);
    }
}

static void sha256(const uint8_t *p, size_t n, uint8_t out[32])
{
    sha256_ctx c;
    sha256_init(&c);
    sha256_update(&c, p, n);
    sha256_final(&c, out);
}

static void to_hex(const uint8_t *b, size_t n, char *out)
{
    static const char hx[] = "0123456789abcdef";
    size_t i;
    for (i = 0; i < n; i++) { out[2*i] = hx[b[i] >> 4]; out[2*i+1] = hx[b[i] & 15]; }
    out[2*n] = '\0';
}

static int sha256_selftest(void)
{
    static const struct { const char *msg; const char *hex; } tv[3] = {
        { "", "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855" },
        { "abc", "ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad" },
        { "abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq",
          "248d6a61d20638b8e5c026930c3e6039a33ce45964ff2167f6ecedd419db06c1" }
    };
    int i;
    for (i = 0; i < 3; i++) {
        uint8_t d[32]; char h[65];
        sha256((const uint8_t *)tv[i].msg, strlen(tv[i].msg), d);
        to_hex(d, 32, h);
        if (strcmp(h, tv[i].hex) != 0) return 0;
    }
    return 1;
}

/* ------------------------------------------------------------------ */
/* AX:MERKLE:v2                                                         */
/* ------------------------------------------------------------------ */
#define N_MAX 1024u
#define L_MAX 10u
static const uint8_t TAG[12] = { 0x41,0x58,0x3A,0x4D,0x45,0x52,0x4B,0x4C,0x45,0x3A,0x76,0x32 };

/* A byte string with an explicit length, so that the "exactly 32 bytes"
 * checks of SHALL-002 step 2 and SHALL-008 step 0 are real checks. */
typedef struct { const uint8_t *p; size_t len; } bstr;

typedef enum {
    V_VERIFIED = 0,
    R_INDEX_OR_SIZE, R_SIZE_MISMATCH, R_PATH_TOO_LONG, R_PATH_TOO_SHORT,
    R_ROOT_MISMATCH, R_NOT_ASCENDING, R_TOO_MANY, R_MALFORMED
} verdict;

static const char *verdict_str(verdict v)
{
    switch (v) {
    case V_VERIFIED:        return "VERIFIED";
    case R_INDEX_OR_SIZE:   return "REJECTED(index_or_size)";
    case R_SIZE_MISMATCH:   return "REJECTED(size_mismatch)";
    case R_PATH_TOO_LONG:   return "REJECTED(path_too_long)";
    case R_PATH_TOO_SHORT:  return "REJECTED(path_too_short)";
    case R_ROOT_MISMATCH:   return "REJECTED(root_mismatch)";
    case R_NOT_ASCENDING:   return "REJECTED(input_not_strictly_ascending)";
    case R_TOO_MANY:        return "REJECTED(too_many_refs)";
    case R_MALFORMED:       return "REJECTED(malformed_reference)";
    }
    return "?";
}

static void le64(uint64_t x, uint8_t out[8])
{
    int i;
    for (i = 0; i < 8; i++) out[i] = (uint8_t)(x >> (8 * i));
}

static void leaf_hash(const uint8_t d[32], uint8_t out[32])
{
    uint8_t buf[33];
    buf[0] = 0x00;
    memcpy(buf + 1, d, 32);
    sha256(buf, sizeof buf, out);
}

static void node_hash(const uint8_t l[32], const uint8_t r[32], uint8_t out[32])
{
    uint8_t buf[65];
    buf[0] = 0x01;
    memcpy(buf + 1, l, 32);
    memcpy(buf + 33, r, 32);
    sha256(buf, sizeof buf, out);
}

/* SHA-256(TAG || LE64(n) || mth) or, if mth == NULL, SHA-256(TAG || LE64(n)). */
static void tagged_root(uint64_t n, const uint8_t *mth, uint8_t out[32])
{
    uint8_t buf[52];
    size_t len = 20;
    memcpy(buf, TAG, 12);
    le64(n, buf + 12);
    if (mth) { memcpy(buf + 20, mth, 32); len = 52; }
    sha256(buf, len, out);
}

/* largest power of two strictly less than n (n > 1) */
static size_t split_k(size_t n)
{
    size_t k = 1;
    while (k * 2 < n) k *= 2;
    return k;
}

/* SHALL-004. Caller guarantees n >= 1 and all elements are 32 bytes. */
static void mth(const bstr *D, size_t n, uint8_t out[32])
{
    if (n == 1) { leaf_hash(D[0].p, out); return; }
    {
        size_t k = split_k(n);
        uint8_t l[32], r[32];
        mth(D, k, l);
        mth(D + k, n - k, r);
        node_hash(l, r, out);
    }
}

/* SHALL-002 checks, in order. Nothing at index >= N_MAX is read. */
static verdict check_input(const bstr *D, size_t n)
{
    size_t i;
    if (n > N_MAX) return R_TOO_MANY;
    for (i = 0; i < n; i++)
        if (D[i].len != 32) return R_MALFORMED;
    for (i = 0; i + 1 < n; i++)
        if (memcmp(D[i].p, D[i + 1].p, 32) >= 0) return R_NOT_ASCENDING;
    return V_VERIFIED;
}

/* SHALL-005, preceded by SHALL-002. Never sorts, never repairs. */
static verdict closure_root(const bstr *D, size_t n, uint8_t out[32])
{
    verdict v = check_input(D, n);
    if (v != V_VERIFIED) return v;
    if (n == 0) { tagged_root(0, NULL, out); return V_VERIFIED; }
    {
        uint8_t h[32];
        mth(D, n, h);
        tagged_root((uint64_t)n, h, out);
    }
    return V_VERIFIED;
}

/* SHALL-007 (recursive core). Appends to path[*L]; cap is capacity. */
static void path_rec(size_t m, const bstr *D, size_t n, uint8_t (*path)[32], size_t *L, size_t cap)
{
    size_t k;
    uint8_t h[32];
    if (n == 1) return;
    k = split_k(n);
    if (m < k) {
        path_rec(m, D, k, path, L, cap);
        mth(D + k, n - k, h);
    } else {
        path_rec(m - k, D + k, n - k, path, L, cap);
        mth(D, k, h);
    }
    if (*L >= cap) { fprintf(stderr, "path capacity exceeded\n"); exit(3); }
    memcpy(path[*L], h, 32);
    (*L)++;
}

/* Path generation as a construction: input checks of SHALL-002 apply,
 * and the proof must be for 0 <= m < n, n >= 1. */
static verdict gen_path(size_t m, const bstr *D, size_t n, uint8_t (*path)[32], size_t *L, size_t cap)
{
    verdict v = check_input(D, n);
    *L = 0;
    if (v != V_VERIFIED) return v;
    if (n < 1 || m >= n) return R_INDEX_OR_SIZE;
    path_rec(m, D, n, path, L, cap);
    return V_VERIFIED;
}

/* SHALL-008. path is an array of L byte strings; only path[0..L-1] is read. */
static verdict verify(bstr d, uint64_t m, uint64_t n, const bstr *path, size_t L,
                      const uint8_t R[32], uint64_t N)
{
    size_t i;
    uint64_t fn, sn;
    uint8_t r[32], t[32], root[32];

    /* 0 */
    if (d.len != 32) return R_MALFORMED;
    for (i = 0; i < L; i++)
        if (path[i].len != 32) return R_MALFORMED;
    /* 1 */
    if (!(n >= 1 && n <= N_MAX && m < n)) return R_INDEX_OR_SIZE;
    /* 1a */
    if (n != N) return R_SIZE_MISMATCH;
    /* 1b */
    if (L > L_MAX) return R_PATH_TOO_LONG;
    /* 2 */
    fn = m; sn = n - 1;
    leaf_hash(d.p, r);
    /* 3 */
    for (i = 0; i < L; i++) {
        const uint8_t *p = path[i].p;
        if (sn == 0) return R_PATH_TOO_LONG;
        if ((fn & 1u) == 1u || fn == sn) {
            node_hash(p, r, t);
            memcpy(r, t, 32);
            while ((fn & 1u) == 0u && fn != 0u) { fn >>= 1; sn >>= 1; }
        } else {
            node_hash(r, p, t);
            memcpy(r, t, 32);
        }
        fn >>= 1; sn >>= 1;
    }
    /* 4 */
    if (sn != 0) return R_PATH_TOO_SHORT;
    /* 5 */
    tagged_root(n, r, root);
    if (memcmp(root, R, 32) != 0) return R_ROOT_MISMATCH;
    /* 6 */
    return V_VERIFIED;
}

/* ------------------------------------------------------------------ */
/* Appendix A: AX:MERKLE:v1 (frozen)                                    */
/* ------------------------------------------------------------------ */
static void v1_root(const bstr *D, size_t n, uint8_t out[32])
{
    uint8_t (*level)[32];
    size_t len, i;
    if (n == 0) { memset(out, 0, 32); return; }
    if (n == 1) { memcpy(out, D[0].p, 32); return; }
    level = malloc((n + 1) * 32);
    if (!level) { fprintf(stderr, "out of memory\n"); exit(3); }
    for (i = 0; i < n; i++) memcpy(level[i], D[i].p, 32);
    len = n;
    while (len > 1) {
        if (len % 2 == 1) { memcpy(level[len], level[len - 1], 32); len++; }
        for (i = 0; i < len / 2; i++) {
            uint8_t buf[64];
            memcpy(buf, level[2*i], 32);
            memcpy(buf + 32, level[2*i + 1], 32);
            sha256(buf, 64, level[i]);
        }
        len /= 2;
    }
    memcpy(out, level[0], 32);
    free(level);
}

/* ------------------------------------------------------------------ */
/* Vector harness (VECTOR-FORMAT.md v0.3)                               */
/* ------------------------------------------------------------------ */
#define MAX_REFS 1025u
static uint8_t REFS[MAX_REFS][32];

static void gen_refs(void)
{
    static const char pfx[] = "AX:MERKLE:v2:vector";
    size_t i, pl = sizeof pfx - 1;
    for (i = 0; i < MAX_REFS; i++) {
        uint8_t buf[64];
        memcpy(buf, pfx, pl);
        le64((uint64_t)i, buf + pl);
        sha256(buf, pl + 8, REFS[i]);
    }
}

static int cmp_bstr(const void *a, const void *b)
{
    return memcmp(((const bstr *)a)->p, ((const bstr *)b)->p, 32);
}

/* D_n: [ref_0 .. ref_{n-1}] sorted ascending (producer-side sort, SHALL-014). */
static bstr *make_D(size_t n)
{
    bstr *D = malloc((n ? n : 1) * sizeof *D);
    size_t i;
    if (!D) { fprintf(stderr, "out of memory\n"); exit(3); }
    for (i = 0; i < n; i++) { D[i].p = REFS[i]; D[i].len = 32; }
    qsort(D, n, sizeof *D, cmp_bstr);
    return D;
}

static void print_hex(const uint8_t *b)
{
    char h[65];
    to_hex(b, 32, h);
    fputs(h, stdout);
}

static void must_ok(verdict v, const char *what)
{
    if (v != V_VERIFIED) {
        fprintf(stderr, "harness: unexpected %s for %s\n", verdict_str(v), what);
        exit(4);
    }
}

#define PCAP 64u
typedef struct { uint8_t h[PCAP][32]; size_t L; bstr b[PCAP]; } pathbuf;

static void P(size_t m, const bstr *D, size_t n, pathbuf *pb)
{
    size_t i;
    must_ok(gen_path(m, D, n, pb->h, &pb->L, PCAP), "PATH");
    for (i = 0; i < pb->L; i++) { pb->b[i].p = pb->h[i]; pb->b[i].len = 32; }
}

static void reject_line(const char *name, verdict v)
{
    printf("REJECT case=%s result=%s\n", name, verdict_str(v));
}

static uint8_t ZERO32[32];

int main(void)
{
    static const size_t ns[] = { 0, 1, 2, 3, 4, 5, 7, 8, 9, 1024 };
    static const size_t big_ms[] = { 0, 511, 512, 1023 };
    size_t ni, i;
    static pathbuf pb, pb2;

    if (!sha256_selftest()) {
        fprintf(stderr, "SHA-256 self-test FAILED\n");
        return 1;
    }
    gen_refs();

    /* Section 1 */
    for (ni = 0; ni < sizeof ns / sizeof ns[0]; ni++) {
        size_t n = ns[ni], cnt, mi;
        bstr *D = make_D(n);
        uint8_t R[32];
        must_ok(closure_root(D, n, R), "closure_root");
        printf("ROOT n=%lu root=", (unsigned long)n);
        print_hex(R);
        putchar('\n');
        cnt = (n <= 9) ? n : 4;
        for (mi = 0; mi < cnt; mi++) {
            size_t m = (n <= 9) ? mi : big_ms[mi];
            verdict v;
            P(m, D, n, &pb);
            v = verify(D[m], m, n, pb.b, pb.L, R, n);
            printf("PROOF n=%lu m=%lu len=%lu path=", (unsigned long)n,
                   (unsigned long)m, (unsigned long)pb.L);
            if (pb.L == 0) putchar('-');
            for (i = 0; i < pb.L; i++) {
                if (i) putchar(',');
                print_hex(pb.h[i]);
            }
            printf(" result=%s\n", verdict_str(v));
        }
        free(D);
    }

    /* Section 2 */
    {
        bstr *D4 = make_D(4), *D9 = make_D(9), *D1024 = make_D(1024), *D1025 = make_D(1025);
        uint8_t R4[32], R9[32], R1024[32], rtmp[32], node01[32], l0[32], l1[32], m23[32], m9[32];
        bstr dnode, one[1];
        verdict v;

        must_ok(closure_root(D4, 4, R4), "R4");
        must_ok(closure_root(D9, 9, R9), "R9");
        must_ok(closure_root(D1024, 1024, R1024), "R1024");
        leaf_hash(D4[0].p, l0);
        leaf_hash(D4[1].p, l1);
        node_hash(l0, l1, node01);
        dnode.p = node01; dnode.len = 32;
        mth(D4 + 2, 2, m23);
        one[0].p = m23; one[0].len = 32;

        /* 1 */
        reject_line("internal_node_as_leaf_short", verify(dnode, 0, 4, one, 1, R4, 4));
        /* 2 */
        reject_line("internal_node_as_leaf_n2", verify(dnode, 0, 2, one, 1, R4, 2));
        /* 3 */
        P(3, D9, 9, &pb);
        reject_line("wrong_index", verify(D9[3], 4, 9, pb.b, pb.L, R9, 9));
        /* 4 */
        reject_line("wrong_size", verify(D9[3], 3, 8, pb.b, pb.L, R9, 8));
        /* 5 */
        if (pb.L < 1) { fprintf(stderr, "harness: empty path\n"); return 4; }
        reject_line("path_short", verify(D9[3], 3, 9, pb.b, pb.L - 1, R9, 9));
        /* 6 */
        pb.b[pb.L].p = ZERO32; pb.b[pb.L].len = 32;
        reject_line("path_long", verify(D9[3], 3, 9, pb.b, pb.L + 1, R9, 9));
        /* 7 */
        P(0, D1024, 1024, &pb2);
        pb2.b[pb2.L].p = ZERO32; pb2.b[pb2.L].len = 32;
        reject_line("path_over_10", verify(D1024[0], 0, 1024, pb2.b, pb2.L + 1, R1024, 1024));
        /* 8 */
        mth(D9, 9, m9);
        tagged_root(8, m9, rtmp);
        reject_line("root_for_other_n", verify(D9[3], 3, 9, pb.b, pb.L, rtmp, 9));
        /* 9 */
        P(0, D9, 9, &pb2);
        reject_line("index_out_of_range", verify(D9[0], 9, 9, pb2.b, pb2.L, R9, 9));
        /* 10 */
        {
            bstr L3[3];
            L3[0] = D9[0]; L3[1] = D9[1]; L3[2] = D9[1];
            v = closure_root(L3, 3, rtmp);
            reject_line("duplicate_ref", v);
        }
        /* 11 */
        {
            bstr L2[2];
            L2[0] = D9[1]; L2[1] = D9[0];
            v = closure_root(L2, 2, rtmp);
            reject_line("non_ascending", v);
        }
        /* 12 */
        reject_line("too_many_refs", closure_root(D1025, 1025, rtmp));
        /* 13 */
        P(0, D4, 4, &pb2);
        reject_line("internal_node_as_leaf_full", verify(dnode, 0, 4, pb2.b, pb2.L, R4, 4));
        /* 14 */
        v1_root(D4, 4, rtmp);
        reject_line("v1_root_presented_as_v2", verify(D4[0], 0, 4, pb2.b, pb2.L, rtmp, 4));
        /* 15 */
        reject_line("size_mismatch", verify(D9[3], 3, 9, pb.b, pb.L, R9, 8));

        free(D4); free(D9); free(D1024); free(D1025);
    }

    /* Section 3 */
    for (ni = 0; ni <= 5; ni++) {
        bstr *D = make_D(ni);
        uint8_t r[32];
        v1_root(D, ni, r);
        printf("V1ROOT n=%lu root=", (unsigned long)ni);
        print_hex(r);
        putchar('\n');
        free(D);
    }
    return 0;
}
