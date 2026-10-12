/*
 * obs_v2.c - clean-room implementation of AX:OBS:v2 (SRS-004-AMD-002 v0.1 DRAFT)
 * and the conformance-vector harness of VECTOR-FORMAT.md.
 * C99, standard library only.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef unsigned char u8;
typedef unsigned long u32x; /* at least 32 bits */
typedef unsigned long long u64;
typedef long long i64;

/* ------------------------------------------------------------------ */
/* SHA-256 (FIPS 180-4)                                                */
/* ------------------------------------------------------------------ */

typedef struct {
    u32x h[8];
    u8 buf[64];
    size_t buflen;
    u64 total; /* bytes */
} sha256_ctx;

static const u32x K256[64] = {
    0x428a2f98UL, 0x71374491UL, 0xb5c0fbcfUL, 0xe9b5dba5UL, 0x3956c25bUL, 0x59f111f1UL, 0x923f82a4UL, 0xab1c5ed5UL,
    0xd807aa98UL, 0x12835b01UL, 0x243185beUL, 0x550c7dc3UL, 0x72be5d74UL, 0x80deb1feUL, 0x9bdc06a7UL, 0xc19bf174UL,
    0xe49b69c1UL, 0xefbe4786UL, 0x0fc19dc6UL, 0x240ca1ccUL, 0x2de92c6fUL, 0x4a7484aaUL, 0x5cb0a9dcUL, 0x76f988daUL,
    0x983e5152UL, 0xa831c66dUL, 0xb00327c8UL, 0xbf597fc7UL, 0xc6e00bf3UL, 0xd5a79147UL, 0x06ca6351UL, 0x14292967UL,
    0x27b70a85UL, 0x2e1b2138UL, 0x4d2c6dfcUL, 0x53380d13UL, 0x650a7354UL, 0x766a0abbUL, 0x81c2c92eUL, 0x92722c85UL,
    0xa2bfe8a1UL, 0xa81a664bUL, 0xc24b8b70UL, 0xc76c51a3UL, 0xd192e819UL, 0xd6990624UL, 0xf40e3585UL, 0x106aa070UL,
    0x19a4c116UL, 0x1e376c08UL, 0x2748774cUL, 0x34b0bcb5UL, 0x391c0cb3UL, 0x4ed8aa4aUL, 0x5b9cca4fUL, 0x682e6ff3UL,
    0x748f82eeUL, 0x78a5636fUL, 0x84c87814UL, 0x8cc70208UL, 0x90befffaUL, 0xa4506cebUL, 0xbef9a3f7UL, 0xc67178f2UL
};

#define M32(x) ((x) & 0xffffffffUL)
#define ROTR(x, n) M32(((x) >> (n)) | ((x) << (32 - (n))))

static void sha256_init(sha256_ctx *c)
{
    c->h[0] = 0x6a09e667UL; c->h[1] = 0xbb67ae85UL; c->h[2] = 0x3c6ef372UL; c->h[3] = 0xa54ff53aUL;
    c->h[4] = 0x510e527fUL; c->h[5] = 0x9b05688cUL; c->h[6] = 0x1f83d9abUL; c->h[7] = 0x5be0cd19UL;
    c->buflen = 0;
    c->total = 0;
}

static void sha256_block(sha256_ctx *c, const u8 *p)
{
    u32x w[64], a, b, cc, d, e, f, g, h, t1, t2;
    int i;
    for (i = 0; i < 16; i++)
        w[i] = ((u32x)p[4 * i] << 24) | ((u32x)p[4 * i + 1] << 16) | ((u32x)p[4 * i + 2] << 8) | (u32x)p[4 * i + 3];
    for (i = 16; i < 64; i++) {
        u32x s0 = ROTR(w[i - 15], 7) ^ ROTR(w[i - 15], 18) ^ (w[i - 15] >> 3);
        u32x s1 = ROTR(w[i - 2], 17) ^ ROTR(w[i - 2], 19) ^ (w[i - 2] >> 10);
        w[i] = M32(w[i - 16] + s0 + w[i - 7] + s1);
    }
    a = c->h[0]; b = c->h[1]; cc = c->h[2]; d = c->h[3];
    e = c->h[4]; f = c->h[5]; g = c->h[6]; h = c->h[7];
    for (i = 0; i < 64; i++) {
        u32x S1 = ROTR(e, 6) ^ ROTR(e, 11) ^ ROTR(e, 25);
        u32x ch = (e & f) ^ (M32(~e) & g);
        u32x S0 = ROTR(a, 2) ^ ROTR(a, 13) ^ ROTR(a, 22);
        u32x maj = (a & b) ^ (a & cc) ^ (b & cc);
        t1 = M32(h + S1 + ch + K256[i] + w[i]);
        t2 = M32(S0 + maj);
        h = g; g = f; f = e; e = M32(d + t1);
        d = cc; cc = b; b = a; a = M32(t1 + t2);
    }
    c->h[0] = M32(c->h[0] + a); c->h[1] = M32(c->h[1] + b);
    c->h[2] = M32(c->h[2] + cc); c->h[3] = M32(c->h[3] + d);
    c->h[4] = M32(c->h[4] + e); c->h[5] = M32(c->h[5] + f);
    c->h[6] = M32(c->h[6] + g); c->h[7] = M32(c->h[7] + h);
}

static void sha256_update(sha256_ctx *c, const u8 *p, size_t n)
{
    c->total += n;
    while (n > 0) {
        size_t take = 64 - c->buflen;
        if (take > n) take = n;
        memcpy(c->buf + c->buflen, p, take);
        c->buflen += take;
        p += take;
        n -= take;
        if (c->buflen == 64) {
            sha256_block(c, c->buf);
            c->buflen = 0;
        }
    }
}

static void sha256_final(sha256_ctx *c, u8 out[32])
{
    u64 bits = c->total * 8u;
    u8 pad = 0x80, zero = 0, len[8];
    int i;
    u64 saved = c->total;
    sha256_update(c, &pad, 1);
    while (c->buflen != 56) sha256_update(c, &zero, 1);
    for (i = 0; i < 8; i++) len[i] = (u8)(bits >> (56 - 8 * i));
    sha256_update(c, len, 8);
    c->total = saved;
    for (i = 0; i < 8; i++) {
        out[4 * i] = (u8)(c->h[i] >> 24);
        out[4 * i + 1] = (u8)(c->h[i] >> 16);
        out[4 * i + 2] = (u8)(c->h[i] >> 8);
        out[4 * i + 3] = (u8)(c->h[i]);
    }
}

static void to_hex(const u8 *d, size_t n, char *out)
{
    static const char hx[] = "0123456789abcdef";
    size_t i;
    for (i = 0; i < n; i++) {
        out[2 * i] = hx[d[i] >> 4];
        out[2 * i + 1] = hx[d[i] & 15];
    }
    out[2 * n] = '\0';
}

static void sha256_hex(const u8 *p, size_t n, char out[65])
{
    sha256_ctx c;
    u8 d[32];
    sha256_init(&c);
    sha256_update(&c, p, n);
    sha256_final(&c, d);
    to_hex(d, 32, out);
}

static int sha256_selftest(void)
{
    static const char *msg[3] = {
        "", "abc", "abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq"
    };
    static const char *exp[3] = {
        "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855",
        "ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad",
        "248d6a61d20638b8e5c026930c3e6039a33ce45964ff2167f6ecedd419db06c1"
    };
    int i;
    char h[65];
    for (i = 0; i < 3; i++) {
        sha256_hex((const u8 *)msg[i], strlen(msg[i]), h);
        if (strcmp(h, exp[i]) != 0) return 0;
    }
    return 1;
}

/* ------------------------------------------------------------------ */
/* UTF-8 validation (well-formed, shortest form, no surrogates, <= 10FFFF) */
/* ------------------------------------------------------------------ */

/* Decode one scalar at p[0..n). Returns bytes consumed, 0 on invalid. */
static size_t utf8_decode(const u8 *p, size_t n, unsigned long *cp)
{
    unsigned long c;
    size_t len, i;
    u8 b0;
    if (n == 0) return 0;
    b0 = p[0];
    if (b0 < 0x80) { *cp = b0; return 1; }
    else if (b0 >= 0xC2 && b0 <= 0xDF) { len = 2; c = b0 & 0x1F; }
    else if (b0 >= 0xE0 && b0 <= 0xEF) { len = 3; c = b0 & 0x0F; }
    else if (b0 >= 0xF0 && b0 <= 0xF4) { len = 4; c = b0 & 0x07; }
    else return 0; /* 80..C1 (continuation or overlong lead), F5..FF */
    if (n < len) return 0;
    for (i = 1; i < len; i++) {
        if ((p[i] & 0xC0) != 0x80) return 0;
        c = (c << 6) | (unsigned long)(p[i] & 0x3F);
    }
    if (len == 3 && c < 0x800) return 0;          /* overlong */
    if (len == 4 && c < 0x10000) return 0;        /* overlong */
    if (c >= 0xD800 && c <= 0xDFFF) return 0;     /* surrogate */
    if (c > 0x10FFFF) return 0;
    *cp = c;
    return len;
}

/* Returns 1 if valid UTF-8 and contains no forbidden control.
 * allow_lf: whether U+000A is permitted. Sets *bad_enc when invalid
 * encoding (as opposed to forbidden control). */
static int utf8_check(const u8 *p, size_t n, int allow_lf, int *bad_enc)
{
    size_t i = 0;
    *bad_enc = 0;
    while (i < n) {
        unsigned long cp;
        size_t k = utf8_decode(p + i, n - i, &cp);
        if (k == 0) { *bad_enc = 1; return 0; }
        if (cp <= 0x1F && !(allow_lf && cp == 0x0A)) return 0;
        if (cp >= 0x7F && cp <= 0x9F) return 0;
        i += k;
    }
    return 1;
}

/* ------------------------------------------------------------------ */
/* Byte buffer                                                         */
/* ------------------------------------------------------------------ */

typedef struct {
    u8 *d;
    size_t n, cap;
} buf_t;

static void buf_put(buf_t *b, const void *p, size_t n)
{
    if (b->n + n > b->cap) {
        size_t nc = b->cap ? b->cap : 256;
        u8 *nd;
        while (nc < b->n + n) nc *= 2;
        nd = (u8 *)realloc(b->d, nc);
        if (!nd) { fputs("out of memory\n", stderr); exit(2); }
        b->d = nd;
        b->cap = nc;
    }
    if (n) memcpy(b->d + b->n, p, n);
    b->n += n;
}

static void buf_str(buf_t *b, const char *s) { buf_put(b, s, strlen(s)); }

/* RFC 8785 string, SHALL-008 table. Input assumed valid UTF-8. */
static void buf_json_string(buf_t *b, const u8 *p, size_t n)
{
    static const char hx[] = "0123456789abcdef";
    size_t i;
    buf_put(b, "\"", 1);
    for (i = 0; i < n; i++) {
        u8 c = p[i];
        switch (c) {
        case '"': buf_put(b, "\\\"", 2); break;
        case '\\': buf_put(b, "\\\\", 2); break;
        case 0x08: buf_put(b, "\\b", 2); break;
        case 0x09: buf_put(b, "\\t", 2); break;
        case 0x0A: buf_put(b, "\\n", 2); break;
        case 0x0C: buf_put(b, "\\f", 2); break;
        case 0x0D: buf_put(b, "\\r", 2); break;
        default:
            if (c < 0x20) {
                char e[6];
                e[0] = '\\'; e[1] = 'u'; e[2] = '0'; e[3] = '0';
                e[4] = hx[c >> 4]; e[5] = hx[c & 15];
                buf_put(b, e, 6);
            } else {
                buf_put(b, &c, 1);
            }
        }
    }
    buf_put(b, "\"", 1);
}

/* SHALL-006 integer form */
static void buf_int(buf_t *b, i64 v)
{
    char tmp[32];
    int k = 0;
    u64 m;
    int neg = v < 0;
    m = neg ? (u64)0 - (u64)v : (u64)v;
    do { tmp[k++] = (char)('0' + (int)(m % 10u)); m /= 10u; } while (m);
    if (neg) tmp[k++] = '-';
    while (k > 0) buf_put(b, &tmp[--k], 1);
}

/* ------------------------------------------------------------------ */
/* Record model                                                        */
/* ------------------------------------------------------------------ */

typedef struct {
    const u8 *p; /* may be NULL */
    size_t n;
} bytes_t;

typedef struct {
    int is_null;
    i64 v;
} nint_t;

typedef struct {
    const char *completion_state;
    const char *failure_type; /* NULL means JSON null */
    bytes_t input;
    i64 ledger_seq;
    bytes_t model_id;
    bytes_t oracle_id;
    bytes_t output;
    i64 output_size;
    nint_t max_tokens, seed, temperature, top_p;
} record_t;

#define MAXSAFE 9007199254740991LL /* 2^53 - 1 */
#define MAX_RECORD 65536u

static int in_range(i64 v, int nonneg)
{
    if (nonneg) return v >= 0 && v <= MAXSAFE;
    return v >= -MAXSAFE && v <= MAXSAFE;
}

static int nint_ok(nint_t x) { return x.is_null || in_range(x.v, 0); }

static void buf_nint(buf_t *b, nint_t x)
{
    if (x.is_null) buf_str(b, "null");
    else buf_int(b, x.v);
}

static int state_pair_ok(const char *cs, const char *ft)
{
    if (strcmp(cs, "COMPLETE") == 0 || strcmp(cs, "TRUNCATED") == 0)
        return ft == NULL;
    if (strcmp(cs, "ERROR") == 0)
        return ft != NULL && (strcmp(ft, "TIMEOUT") == 0 || strcmp(ft, "INVALID_OUTPUT") == 0 ||
                              strcmp(ft, "TRANSPORT_ERROR") == 0);
    return 0;
}

static int identifier_ok(bytes_t s)
{
    int bad;
    if (s.n == 0 || s.n > 256) return 0;
    return utf8_check(s.p, s.n, 0, &bad);
}

/* Canonical encoding. with_obs: include obs_hash member. */
static void encode(buf_t *b, const record_t *r, const char *input_hash, const char *obs_hash)
{
    buf_str(b, "{\"completion_state\":");
    buf_json_string(b, (const u8 *)r->completion_state, strlen(r->completion_state));
    buf_str(b, ",\"failure_type\":");
    if (r->failure_type) buf_json_string(b, (const u8 *)r->failure_type, strlen(r->failure_type));
    else buf_str(b, "null");
    buf_str(b, ",\"input_hash\":");
    buf_json_string(b, (const u8 *)input_hash, 64);
    buf_str(b, ",\"ledger_seq\":");
    buf_int(b, r->ledger_seq);
    buf_str(b, ",\"model_id\":");
    buf_json_string(b, r->model_id.p, r->model_id.n);
    if (obs_hash) {
        buf_str(b, ",\"obs_hash\":");
        buf_json_string(b, (const u8 *)obs_hash, 64);
    }
    buf_str(b, ",\"oracle_id\":");
    buf_json_string(b, r->oracle_id.p, r->oracle_id.n);
    buf_str(b, ",\"output\":");
    buf_json_string(b, r->output.p, r->output.n);
    buf_str(b, ",\"output_size\":");
    buf_int(b, r->output_size);
    buf_str(b, ",\"params\":{\"max_tokens\":");
    buf_nint(b, r->max_tokens);
    buf_str(b, ",\"seed\":");
    buf_nint(b, r->seed);
    buf_str(b, ",\"temperature\":");
    buf_nint(b, r->temperature);
    buf_str(b, ",\"top_p\":");
    buf_nint(b, r->top_p);
    buf_str(b, "},\"schema_version\":\"AX:OBS:v2\"}");
}

/* Admission + production. Returns NULL on success (stored record in *out,
 * obs_hash in obs_hex), or the rejection reason. */
static const char *admit(const record_t *r, buf_t *out, char obs_hex[65])
{
    int bad;
    char in_hex[65];
    buf_t P = {NULL, 0, 0};
    sha256_ctx c;
    u8 d[32], le[8];
    int i;

    /* 1 state_pair */
    if (!state_pair_ok(r->completion_state, r->failure_type)) return "state_pair";
    /* 2 integer_range */
    if (!in_range(r->ledger_seq, 1)) return "integer_range";
    if (!in_range(r->output_size, 1)) return "integer_range";
    if (!nint_ok(r->max_tokens) || !nint_ok(r->seed) || !nint_ok(r->temperature) || !nint_ok(r->top_p))
        return "integer_range";
    /* 3 identifier */
    if (!identifier_ok(r->model_id)) return "identifier";
    if (!identifier_ok(r->oracle_id)) return "identifier";
    /* 4 input */
    if (r->input.p == NULL && r->input.n != 0) return "input";
    /* 5 output_encoding */
    if (!utf8_check(r->output.p, r->output.n, 1, &bad)) return "output_encoding";
    /* 6 output_size */
    if (strcmp(r->completion_state, "TRUNCATED") == 0) {
        if ((u64)r->output_size < (u64)r->output.n) return "output_size";
    } else {
        if ((u64)r->output_size != (u64)r->output.n) return "output_size";
    }
    /* 7, 8 (NFC) skipped as VECTOR-FORMAT.md step 1 permits. */

    /* hashes */
    sha256_hex(r->input.n ? r->input.p : (const u8 *)"", r->input.n, in_hex);
    encode(&P, r, in_hex, NULL);
    sha256_init(&c);
    sha256_update(&c, (const u8 *)"AX:OBS:v2", 9);
    for (i = 0; i < 8; i++) le[i] = (u8)((u64)P.n >> (8 * i));
    sha256_update(&c, le, 8);
    sha256_update(&c, P.d, P.n);
    sha256_final(&c, d);
    to_hex(d, 32, obs_hex);
    free(P.d);

    encode(out, r, in_hex, obs_hex);
    /* 9 record_size */
    if (out->n > MAX_RECORD) return "record_size";
    return NULL;
}

/* ------------------------------------------------------------------ */
/* Harness                                                             */
/* ------------------------------------------------------------------ */

static bytes_t S(const char *s) { bytes_t b; b.p = (const u8 *)s; b.n = strlen(s); return b; }
static nint_t NUL(void) { nint_t x; x.is_null = 1; x.v = 0; return x; }
static nint_t NI(i64 v) { nint_t x; x.is_null = 0; x.v = v; return x; }

static record_t base(void)
{
    record_t r;
    r.completion_state = "COMPLETE";
    r.failure_type = NULL;
    r.input = S("prompt");
    r.ledger_seq = 1;
    r.model_id = S("m");
    r.oracle_id = S("o");
    r.output = S("ok");
    r.output_size = -1; /* sentinel: derive from output */
    r.max_tokens = NUL(); r.seed = NUL(); r.temperature = NUL(); r.top_p = NUL();
    return r;
}

static int run_case(const char *name, record_t r)
{
    buf_t out = {NULL, 0, 0};
    char obs[65], rec[65];
    const char *why;
    if (r.output_size == -1) r.output_size = (i64)r.output.n;
    why = admit(&r, &out, obs);
    if (why) {
        printf("REJECT case=%s result=REJECTED(%s)\n", name, why);
    } else {
        sha256_hex(out.d, out.n, rec);
        printf("OBS case=%s len=%lu record_sha256=%s obs_hash=%s\n", name, (unsigned long)out.n, rec, obs);
    }
    free(out.d);
    return 0;
}

static u8 *repeat(const char *unit, size_t count, bytes_t *b)
{
    size_t ul = strlen(unit), i;
    u8 *p = (u8 *)malloc(ul * count + 1);
    if (!p) { fputs("out of memory\n", stderr); exit(2); }
    for (i = 0; i < count; i++) memcpy(p + i * ul, unit, ul);
    b->p = p;
    b->n = ul * count;
    return p;
}

int main(void)
{
    record_t r;
    u8 *heap;
    static const u8 bad_utf8[3] = {0x61, 0xff, 0x62};

    if (!sha256_selftest()) {
        fputs("SHA-256 self-test failed\n", stderr);
        return 1;
    }

    /* Section 1 */
    r = base(); run_case("base", r);
    r = base(); r.input = S(""); run_case("empty_input", r);
    r = base(); r.output = S("a\nb"); run_case("lf_output", r);
    r = base(); r.output = S("say \"hi\" \\ done"); run_case("quote_backslash", r);
    r = base(); r.output = S("caf\xC3\xA9 \xC3\x85 \xF0\x9F\x98\x80"); run_case("non_ascii", r);
    r = base(); r.max_tokens = NI(256); r.seed = NI(9007199254740991LL);
    r.temperature = NI(45875); r.top_p = NI(62259); run_case("params_full", r);
    r = base(); r.seed = NI(-9007199254740991LL); r.temperature = NI(-65536); r.top_p = NI(0);
    run_case("params_negative", r);
    r = base(); r.completion_state = "TRUNCATED"; r.output = S("abc"); r.output_size = 100000;
    run_case("truncated", r);
    r = base(); r.completion_state = "ERROR"; r.failure_type = "TIMEOUT"; r.output = S("");
    run_case("error_timeout", r);
    r = base(); r.completion_state = "ERROR"; r.failure_type = "INVALID_OUTPUT"; r.output = S("");
    run_case("error_invalid_output", r);
    r = base(); r.completion_state = "ERROR"; r.failure_type = "TRANSPORT_ERROR"; r.output = S("");
    run_case("error_transport", r);
    r = base(); r.ledger_seq = 9007199254740991LL; run_case("ledger_max", r);
    r = base(); r.ledger_seq = 0; run_case("ledger_zero", r);
    r = base(); r.model_id = S("mod\xC3\xA8le-1"); r.oracle_id = S("\xCF\x89-oracle");
    run_case("identifiers_utf8", r);
    r = base(); heap = repeat("m", 256, &r.model_id); run_case("identifier_256", r); free(heap);
    r = base(); heap = repeat("\"", 32000, &r.output); run_case("quote_heavy_fits", r); free(heap);
    r = base(); r.output = S("\xC2\xA0\xE2\x80\xA8\xE2\x80\xA9"); run_case("del_free_unicode", r);

    /* Section 2 */
    r = base(); r.ledger_seq = 9007199254740992LL; run_case("ledger_over", r);
    r = base(); r.seed = NI(9007199254740992LL); run_case("seed_over", r);
    r = base(); r.seed = NI(-9007199254740992LL); run_case("seed_under", r);
    r = base(); r.completion_state = "TRUNCATED"; r.output_size = 9007199254740992LL;
    run_case("output_size_over", r);
    r = base(); r.failure_type = "TIMEOUT"; run_case("complete_with_timeout", r);
    r = base(); r.completion_state = "ERROR"; r.output = S(""); run_case("error_without_failure", r);
    r = base(); r.completion_state = "TRUNCATED"; r.failure_type = "INVALID_OUTPUT";
    run_case("truncated_with_failure", r);
    r = base(); r.completion_state = "PARTIAL"; run_case("unknown_state", r);
    r = base(); r.model_id = S(""); run_case("empty_model_id", r);
    r = base(); r.oracle_id = S("o\x01"); run_case("control_in_oracle_id", r);
    r = base(); r.model_id = S("m\xC2\x85"); run_case("c1_in_model_id", r);
    r = base(); heap = repeat("m", 257, &r.model_id); run_case("model_id_257", r); free(heap);
    r = base(); r.input.p = NULL; r.input.n = 5; run_case("null_input", r);
    r = base(); r.output = S("a\x01" "b"); run_case("control_in_output", r);
    r = base(); r.output = S("a\rb"); run_case("cr_in_output", r);
    r = base(); r.output = S("x\x7fy"); run_case("del_in_output", r);
    r = base(); r.output.p = bad_utf8; r.output.n = 3; run_case("invalid_utf8_output", r);
    r = base(); r.output = S("ok"); r.output_size = 3; run_case("size_mismatch", r);
    r = base(); r.completion_state = "TRUNCATED"; r.output = S("abc"); r.output_size = 2;
    run_case("truncated_size_under", r);
    r = base(); heap = repeat("\"", 33000, &r.output); run_case("quote_heavy_over", r); free(heap);
    r = base(); r.failure_type = "TIMEOUT"; r.ledger_seq = 9007199254740992LL;
    run_case("order_state_before_range", r);
    r = base(); r.model_id = S(""); r.output = S("a\x01" "b"); run_case("order_identifier_before_output", r);

    return fflush(stdout) == 0 ? 0 : 1;
}
