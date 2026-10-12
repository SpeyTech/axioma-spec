/* AX:MERKLE:v2 (SRS-007-AMD-001) clean-room implementation and vector generator.
 * Portable C99, no external libraries. SHA-256 per FIPS 180-4. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

/* ---------------- SHA-256 (FIPS 180-4) ---------------- */
typedef struct {
    uint32_t h[8];
    uint64_t len;          /* total bytes processed */
    unsigned char buf[64];
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
#define CH(x,y,z) (((x) & (y)) ^ (~(x) & (z)))
#define MAJ(x,y,z) (((x) & (y)) ^ ((x) & (z)) ^ ((y) & (z)))
#define BSIG0(x) (ROTR(x,2) ^ ROTR(x,13) ^ ROTR(x,22))
#define BSIG1(x) (ROTR(x,6) ^ ROTR(x,11) ^ ROTR(x,25))
#define SSIG0(x) (ROTR(x,7) ^ ROTR(x,18) ^ ((x) >> 3))
#define SSIG1(x) (ROTR(x,17) ^ ROTR(x,19) ^ ((x) >> 10))

static void sha256_block(sha256_ctx *c, const unsigned char *p)
{
    uint32_t w[64], a, b, cc, d, e, f, g, h, t1, t2;
    int t;
    for (t = 0; t < 16; t++)
        w[t] = ((uint32_t)p[4*t] << 24) | ((uint32_t)p[4*t+1] << 16) |
               ((uint32_t)p[4*t+2] << 8) | (uint32_t)p[4*t+3];
    for (t = 16; t < 64; t++)
        w[t] = SSIG1(w[t-2]) + w[t-7] + SSIG0(w[t-15]) + w[t-16];
    a = c->h[0]; b = c->h[1]; cc = c->h[2]; d = c->h[3];
    e = c->h[4]; f = c->h[5]; g = c->h[6]; h = c->h[7];
    for (t = 0; t < 64; t++) {
        t1 = h + BSIG1(e) + CH(e,f,g) + K256[t] + w[t];
        t2 = BSIG0(a) + MAJ(a,b,cc);
        h = g; g = f; f = e; e = d + t1; d = cc; cc = b; b = a; a = t1 + t2;
    }
    c->h[0] += a; c->h[1] += b; c->h[2] += cc; c->h[3] += d;
    c->h[4] += e; c->h[5] += f; c->h[6] += g; c->h[7] += h;
}

static void sha256_init(sha256_ctx *c)
{
    static const uint32_t H0[8] = {
        0x6a09e667u,0xbb67ae85u,0x3c6ef372u,0xa54ff53au,
        0x510e527fu,0x9b05688cu,0x1f83d9abu,0x5be0cd19u };
    memcpy(c->h, H0, sizeof H0);
    c->len = 0; c->blen = 0;
}

static void sha256_update(sha256_ctx *c, const unsigned char *p, size_t n)
{
    c->len += n;
    while (n > 0) {
        size_t take = 64 - c->blen;
        if (take > n) take = n;
        memcpy(c->buf + c->blen, p, take);
        c->blen += take; p += take; n -= take;
        if (c->blen == 64) { sha256_block(c, c->buf); c->blen = 0; }
    }
}

static void sha256_final(sha256_ctx *c, unsigned char out[32])
{
    uint64_t bits = c->len * 8u;
    unsigned char pad = 0x80, zero = 0, lenb[8];
    int i;
    sha256_update(c, &pad, 1);
    while (c->blen != 56) sha256_update(c, &zero, 1);
    for (i = 0; i < 8; i++) lenb[i] = (unsigned char)(bits >> (56 - 8*i));
    sha256_update(c, lenb, 8);
    for (i = 0; i < 8; i++) {
        out[4*i]   = (unsigned char)(c->h[i] >> 24);
        out[4*i+1] = (unsigned char)(c->h[i] >> 16);
        out[4*i+2] = (unsigned char)(c->h[i] >> 8);
        out[4*i+3] = (unsigned char)(c->h[i]);
    }
}

static void sha256(const unsigned char *p, size_t n, unsigned char out[32])
{
    sha256_ctx c; sha256_init(&c); sha256_update(&c, p, n); sha256_final(&c, out);
}

static void tohex(const unsigned char *p, size_t n, char *out)
{
    static const char hx[] = "0123456789abcdef";
    size_t i;
    for (i = 0; i < n; i++) { out[2*i] = hx[p[i] >> 4]; out[2*i+1] = hx[p[i] & 15]; }
    out[2*n] = 0;
}

static int sha256_selftest(void)
{
    static const char *msg[3] = {
        "", "abc", "abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq" };
    static const char *exp[3] = {
        "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855",
        "ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad",
        "248d6a61d20638b8e5c026930c3e6039a33ce45964ff2167f6ecedd419db06c1" };
    unsigned char d[32]; char hx[65]; int i;
    for (i = 0; i < 3; i++) {
        sha256((const unsigned char *)msg[i], strlen(msg[i]), d);
        tohex(d, 32, hx);
        if (strcmp(hx, exp[i]) != 0) return 0;
    }
    return 1;
}

/* ---------------- AX:MERKLE:v2 ---------------- */
#define N_MAX 1024
#define PATH_MAX_LEN 10
#define HL 32

static const unsigned char TAG[12] = {
    0x41,0x58,0x3A,0x4D,0x45,0x52,0x4B,0x4C,0x45,0x3A,0x76,0x32 };

typedef unsigned char ref_t[HL];

static void le64(uint64_t x, unsigned char out[8])
{
    int i; for (i = 0; i < 8; i++) out[i] = (unsigned char)(x >> (8*i));
}

static void leaf_hash(const unsigned char d[HL], unsigned char out[HL])
{
    unsigned char b[1 + HL];
    b[0] = 0x00; memcpy(b + 1, d, HL);
    sha256(b, sizeof b, out);
}

static void node_hash(const unsigned char l[HL], const unsigned char r[HL], unsigned char out[HL])
{
    unsigned char b[1 + 2*HL];
    b[0] = 0x01; memcpy(b + 1, l, HL); memcpy(b + 1 + HL, r, HL);
    sha256(b, sizeof b, out);
}

static size_t split_k(size_t n) /* largest power of two strictly less than n, n > 1 */
{
    size_t k = 1;
    while (k * 2 < n) k *= 2;
    return k;
}

/* MTH over D[0:n], n >= 1. Recursion depth <= log2(N_MAX)+1. */
static void mth(ref_t *D, size_t n, unsigned char out[HL])
{
    unsigned char a[HL], b[HL];
    size_t k;
    if (n == 1) { leaf_hash(D[0], out); return; }
    k = split_k(n);
    mth(D, k, a);
    mth(D + k, n - k, b);
    node_hash(a, b, out);
}

static void tagged_root(uint64_t n, const unsigned char *mthv, unsigned char out[HL])
{
    unsigned char b[12 + 8 + HL];
    size_t len = 12 + 8;
    memcpy(b, TAG, 12); le64(n, b + 12);
    if (mthv) { memcpy(b + 20, mthv, HL); len += HL; }
    sha256(b, len, out);
}

enum { OK = 0, R_INDEX_OR_SIZE, R_PATH_TOO_LONG, R_PATH_TOO_SHORT, R_ROOT_MISMATCH,
       R_NOT_ASC, R_TOO_MANY };

static const char *verdict(int v)
{
    switch (v) {
    case OK: return "VERIFIED";
    case R_INDEX_OR_SIZE: return "REJECTED(index_or_size)";
    case R_PATH_TOO_LONG: return "REJECTED(path_too_long)";
    case R_PATH_TOO_SHORT: return "REJECTED(path_too_short)";
    case R_ROOT_MISMATCH: return "REJECTED(root_mismatch)";
    case R_NOT_ASC: return "REJECTED(input_not_strictly_ascending)";
    case R_TOO_MANY: return "REJECTED(too_many_refs)";
    default: return "?";
    }
}

/* SHALL-002/005. n is checked before any element is read. */
static int closure_root(ref_t *D, size_t n, unsigned char out[HL])
{
    unsigned char m[HL];
    size_t i;
    if (n > N_MAX) return R_TOO_MANY;
    for (i = 1; i < n; i++)
        if (memcmp(D[i-1], D[i], HL) >= 0) return R_NOT_ASC;
    if (n == 0) { tagged_root(0, NULL, out); return OK; }
    mth(D, n, m);
    tagged_root((uint64_t)n, m, out);
    return OK;
}

/* SHALL-007. Appends into path (capacity PATH_MAX_LEN); returns new length. */
static size_t gen_path(size_t m, ref_t *D, size_t n, ref_t *path, size_t len)
{
    size_t k;
    if (n == 1) return len;
    k = split_k(n);
    if (m < k) {
        len = gen_path(m, D, k, path, len);
        if (len < PATH_MAX_LEN) mth(D + k, n - k, path[len]);
    } else {
        len = gen_path(m - k, D + k, n - k, path, len);
        if (len < PATH_MAX_LEN) mth(D, k, path[len]);
    }
    return len + 1;
}

/* SHALL-008. Reads at most plen entries of path, and none if plen > 10. */
static int verify(const unsigned char d[HL], size_t m, size_t n,
                  ref_t *path, size_t plen, const unsigned char R[HL])
{
    size_t fn, sn, i;
    unsigned char r[HL], t[HL];
    if (!(n >= 1 && n <= N_MAX && m < n)) return R_INDEX_OR_SIZE;
    if (plen > PATH_MAX_LEN) return R_PATH_TOO_LONG;
    fn = m; sn = n - 1;
    leaf_hash(d, r);
    for (i = 0; i < plen; i++) {
        if (sn == 0) return R_PATH_TOO_LONG;
        if ((fn & 1u) == 1u || fn == sn) {
            node_hash(path[i], r, t); memcpy(r, t, HL);
            while ((fn & 1u) == 0 && fn != 0) { fn >>= 1; sn >>= 1; }
        } else {
            node_hash(r, path[i], t); memcpy(r, t, HL);
        }
        fn >>= 1; sn >>= 1;
    }
    if (sn != 0) return R_PATH_TOO_SHORT;
    tagged_root((uint64_t)n, r, t);
    if (memcmp(t, R, HL) != 0) return R_ROOT_MISMATCH;
    return OK;
}

/* ---------------- vector harness ---------------- */
static ref_t g_refs[N_MAX + 1];

static int refcmp(const void *a, const void *b) { return memcmp(a, b, HL); }

static void make_D(size_t n, ref_t *out)
{
    static const char pre[] = "AX:MERKLE:v2:vector";
    unsigned char b[sizeof pre - 1 + 8];
    size_t i;
    for (i = 0; i < n; i++) {
        memcpy(b, pre, sizeof pre - 1);
        le64((uint64_t)i, b + sizeof pre - 1);
        sha256(b, sizeof b, out[i]);
    }
    qsort(out, n, HL, refcmp);
}

static void print_hex(const unsigned char *p)
{
    char hx[2*HL + 1]; tohex(p, HL, hx); fputs(hx, stdout);
}

static void print_reject(const char *name, int v)
{
    printf("REJECT case=%s result=%s\n", name, verdict(v));
}

int main(void)
{
    static const size_t NS[10] = { 0, 1, 2, 3, 4, 5, 7, 8, 9, 1024 };
    static const size_t BIGM[4] = { 0, 511, 512, 1023 };
    static ref_t D[N_MAX + 1], D4[4], D9[9];
    ref_t path[PATH_MAX_LEN + 1];
    unsigned char root[HL], R9[HL], R4[HL], R1024[HL], x[HL], y[HL];
    size_t ni, j, plen;
    int v;

    if (!sha256_selftest()) { fprintf(stderr, "SHA-256 self-test FAILED\n"); return 1; }

    /* Section 1 */
    for (ni = 0; ni < 10; ni++) {
        size_t n = NS[ni], cnt = (n <= 9) ? n : 4;
        make_D(n, D);
        if (closure_root(D, n, root) != OK) { fprintf(stderr, "unexpected\n"); return 1; }
        printf("ROOT n=%lu root=", (unsigned long)n); print_hex(root); putchar('\n');
        for (j = 0; j < cnt; j++) {
            size_t m = (n <= 9) ? j : BIGM[j], i;
            plen = gen_path(m, D, n, path, 0);
            if (plen > PATH_MAX_LEN) { fprintf(stderr, "path overflow\n"); return 1; }
            v = verify(D[m], m, n, path, plen, root);
            printf("PROOF n=%lu m=%lu len=%lu path=", (unsigned long)n, (unsigned long)m,
                   (unsigned long)plen);
            if (plen == 0) putchar('-');
            for (i = 0; i < plen; i++) { if (i) putchar(','); print_hex(path[i]); }
            printf(" result=%s\n", verdict(v));
        }
    }

    /* Section 2 */
    make_D(4, D4); closure_root(D4, 4, R4);
    make_D(9, D9); closure_root(D9, 9, R9);
    make_D(1024, D); closure_root(D, 1024, R1024);

    /* 1, 2: internal node as leaf */
    leaf_hash(D4[0], x); leaf_hash(D4[1], y); node_hash(x, y, x);
    mth(D4 + 2, 2, path[0]);
    print_reject("internal_node_as_leaf_short", verify(x, 0, 4, path, 1, R4));
    print_reject("internal_node_as_leaf_n2", verify(x, 0, 2, path, 1, R4));

    /* 3-6, 8, 9 on D_9 */
    plen = gen_path(3, D9, 9, path, 0);
    print_reject("wrong_index", verify(D9[3], 4, 9, path, plen, R9));
    print_reject("wrong_size", verify(D9[3], 3, 8, path, plen, R9));
    print_reject("path_short", verify(D9[3], 3, 9, path, plen - 1, R9));
    memset(path[plen], 0, HL);
    print_reject("path_long", verify(D9[3], 3, 9, path, plen + 1, R9));

    /* 7: path over 10 */
    {
        size_t pl = gen_path(0, D, 1024, path, 0);
        if (pl != PATH_MAX_LEN) { fprintf(stderr, "unexpected len\n"); return 1; }
        memset(path[pl], 0, HL);
        print_reject("path_over_10", verify(D[0], 0, 1024, path, pl + 1, R1024));
    }

    plen = gen_path(3, D9, 9, path, 0);
    mth(D9, 9, x); tagged_root(8, x, y);
    print_reject("root_for_other_n", verify(D9[3], 3, 9, path, plen, y));

    plen = gen_path(0, D9, 9, path, 0);
    print_reject("index_out_of_range", verify(D9[0], 9, 9, path, plen, R9));

    /* 10-12: construction rejections */
    {
        ref_t L[3];
        memcpy(L[0], D9[0], HL); memcpy(L[1], D9[1], HL); memcpy(L[2], D9[1], HL);
        print_reject("duplicate_ref", closure_root(L, 3, root));
        memcpy(L[0], D9[1], HL); memcpy(L[1], D9[0], HL);
        print_reject("non_ascending", closure_root(L, 2, root));
    }
    make_D(N_MAX + 1, g_refs);
    print_reject("too_many_refs", closure_root(g_refs, N_MAX + 1, root));
    return 0;
}
