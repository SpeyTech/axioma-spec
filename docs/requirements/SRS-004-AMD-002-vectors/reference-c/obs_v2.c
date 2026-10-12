/*
 * obs_v2.c - clean-room implementation of the AX:OBS:v2 record format
 * (SRS-004-AMD-002 v0.2 DRAFT) and its conformance vector harness
 * (VECTOR-FORMAT.md). C99, standard library only.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ------------------------------------------------------------------ */
/* SHA-256 (FIPS 180-4)                                                */
/* ------------------------------------------------------------------ */

typedef unsigned long u32; /* at least 32 bits; masked explicitly */

typedef struct {
    u32 h[8];
    unsigned char buf[64];
    size_t buflen;
    unsigned long long total; /* bytes */
} sha256_ctx;

static const u32 K256[64] = {
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

static void sha256_block(sha256_ctx *c, const unsigned char *p)
{
    u32 w[64], a, b, cc, d, e, f, g, h, t1, t2;
    int i;
    for (i = 0; i < 16; i++)
        w[i] = ((u32)p[4 * i] << 24) | ((u32)p[4 * i + 1] << 16) | ((u32)p[4 * i + 2] << 8) | (u32)p[4 * i + 3];
    for (i = 16; i < 64; i++) {
        u32 s0 = ROTR(w[i - 15], 7) ^ ROTR(w[i - 15], 18) ^ (w[i - 15] >> 3);
        u32 s1 = ROTR(w[i - 2], 17) ^ ROTR(w[i - 2], 19) ^ (w[i - 2] >> 10);
        w[i] = M32(w[i - 16] + s0 + w[i - 7] + s1);
    }
    a = c->h[0]; b = c->h[1]; cc = c->h[2]; d = c->h[3];
    e = c->h[4]; f = c->h[5]; g = c->h[6]; h = c->h[7];
    for (i = 0; i < 64; i++) {
        u32 S1 = ROTR(e, 6) ^ ROTR(e, 11) ^ ROTR(e, 25);
        u32 ch = (e & f) ^ (M32(~e) & g);
        u32 S0 = ROTR(a, 2) ^ ROTR(a, 13) ^ ROTR(a, 22);
        u32 mj = (a & b) ^ (a & cc) ^ (b & cc);
        t1 = M32(h + S1 + ch + K256[i] + w[i]);
        t2 = M32(S0 + mj);
        h = g; g = f; f = e; e = M32(d + t1);
        d = cc; cc = b; b = a; a = M32(t1 + t2);
    }
    c->h[0] = M32(c->h[0] + a); c->h[1] = M32(c->h[1] + b);
    c->h[2] = M32(c->h[2] + cc); c->h[3] = M32(c->h[3] + d);
    c->h[4] = M32(c->h[4] + e); c->h[5] = M32(c->h[5] + f);
    c->h[6] = M32(c->h[6] + g); c->h[7] = M32(c->h[7] + h);
}

static void sha256_update(sha256_ctx *c, const unsigned char *p, size_t n)
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

static void sha256_final(sha256_ctx *c, unsigned char out[32])
{
    unsigned long long bits = c->total * 8ULL;
    unsigned char pad = 0x80, zero = 0, lenb[8];
    int i;
    unsigned long long saved = c->total;
    sha256_update(c, &pad, 1);
    while (c->buflen != 56) sha256_update(c, &zero, 1);
    for (i = 0; i < 8; i++) lenb[i] = (unsigned char)((bits >> (56 - 8 * i)) & 0xff);
    sha256_update(c, lenb, 8);
    c->total = saved;
    for (i = 0; i < 8; i++) {
        out[4 * i] = (unsigned char)((c->h[i] >> 24) & 0xff);
        out[4 * i + 1] = (unsigned char)((c->h[i] >> 16) & 0xff);
        out[4 * i + 2] = (unsigned char)((c->h[i] >> 8) & 0xff);
        out[4 * i + 3] = (unsigned char)(c->h[i] & 0xff);
    }
}

static void to_hex(const unsigned char *d, size_t n, char *out)
{
    static const char hx[] = "0123456789abcdef";
    size_t i;
    for (i = 0; i < n; i++) {
        out[2 * i] = hx[d[i] >> 4];
        out[2 * i + 1] = hx[d[i] & 15];
    }
    out[2 * n] = '\0';
}

static void sha256_hex(const unsigned char *p, size_t n, char out[65])
{
    sha256_ctx c;
    unsigned char d[32];
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
    char h[65];
    int i;
    for (i = 0; i < 3; i++) {
        sha256_hex((const unsigned char *)msg[i], strlen(msg[i]), h);
        if (strcmp(h, exp[i]) != 0) return 0;
    }
    /* also check incremental updates agree with one-shot on the 448-bit message */
    {
        sha256_ctx c;
        unsigned char d[32];
        size_t k, n = strlen(msg[2]);
        sha256_init(&c);
        for (k = 0; k < n; k++) sha256_update(&c, (const unsigned char *)msg[2] + k, 1);
        sha256_final(&c, d);
        to_hex(d, 32, h);
        if (strcmp(h, exp[2]) != 0) return 0;
    }
    return 1;
}

/* ------------------------------------------------------------------ */
/* Strict UTF-8 decoding (SHALL-012 bullet 1)                          */
/* ------------------------------------------------------------------ */

/*
 * Decode one code point at s[i..n). Returns the sequence length (1..4)
 * and stores the code point, or 0 if the bytes are not well-formed
 * shortest-form UTF-8 for a scalar value at most U+10FFFF.
 */
static int utf8_next(const unsigned char *s, size_t n, size_t i, unsigned long *cp)
{
    unsigned char b0 = s[i];
    size_t len, k;
    unsigned long v, min;
    if (b0 < 0x80) { *cp = b0; return 1; }
    if (b0 >= 0xC2 && b0 <= 0xDF) { len = 2; v = b0 & 0x1F; min = 0x80; }
    else if (b0 >= 0xE0 && b0 <= 0xEF) { len = 3; v = b0 & 0x0F; min = 0x800; }
    else if (b0 >= 0xF0 && b0 <= 0xF4) { len = 4; v = b0 & 0x07; min = 0x10000; }
    else return 0; /* continuation byte, C0, C1 or F5..FF lead */
    if (n - i < len) return 0;
    for (k = 1; k < len; k++) {
        unsigned char b = s[i + k];
        if ((b & 0xC0) != 0x80) return 0;
        v = (v << 6) | (unsigned long)(b & 0x3F);
    }
    if (v < min) return 0;                       /* overlong */
    if (v >= 0xD800 && v <= 0xDFFF) return 0;    /* surrogate */
    if (v > 0x10FFFF) return 0;                  /* beyond Unicode */
    *cp = v;
    return (int)len;
}

static int utf8_valid(const unsigned char *s, size_t n)
{
    size_t i = 0;
    unsigned long cp;
    while (i < n) {
        int l = utf8_next(s, n, i, &cp);
        if (l == 0) return 0;
        i += (size_t)l;
    }
    return 1;
}

/* ------------------------------------------------------------------ */
/* Byte buffer                                                         */
/* ------------------------------------------------------------------ */

typedef struct {
    unsigned char *p;
    size_t len, cap;
} buf_t;

static void buf_put(buf_t *b, const void *d, size_t n)
{
    if (b->len + n > b->cap) {
        size_t nc = b->cap ? b->cap : 256;
        while (nc < b->len + n) nc *= 2;
        b->p = (unsigned char *)realloc(b->p, nc);
        if (!b->p) { fprintf(stderr, "out of memory\n"); exit(2); }
        b->cap = nc;
    }
    memcpy(b->p + b->len, d, n);
    b->len += n;
}

static void buf_str(buf_t *b, const char *s) { buf_put(b, s, strlen(s)); }

/* ------------------------------------------------------------------ */
/* Record model                                                        */
/* ------------------------------------------------------------------ */

#define MAX_SAFE 9007199254740991LL /* 2^53 - 1 */
#define MAX_I32 2147483647LL
#define AX_MAX_OBS_BYTES 65536

typedef struct {
    int present; /* 0 means null */
    long long v;
} opt_int;

typedef struct {
    const char *completion_state;
    const char *failure_type; /* NULL means JSON null */
    const unsigned char *input; /* NULL means absent */
    size_t input_len;
    long long ledger_seq;
    const unsigned char *model_id; size_t model_id_len;
    const unsigned char *oracle_id; size_t oracle_id_len;
    const unsigned char *output; size_t output_len;
    long long output_size;
    opt_int max_tokens, seed, temperature, top_p;
    int has_rrs;
    long long rrs;
} record_t;

/* Canonical integer form (SHALL-006): decimal, '-' for negatives, no '+', no leading zeros, never -0. */
static void put_int(buf_t *b, long long v)
{
    char tmp[32];
    int n = 0;
    unsigned long long u;
    if (v < 0) { buf_put(b, "-", 1); u = (unsigned long long)(-(v + 1)) + 1ULL; }
    else u = (unsigned long long)v;
    do { tmp[n++] = (char)('0' + (int)(u % 10ULL)); u /= 10ULL; } while (u);
    while (n > 0) { n--; buf_put(b, &tmp[n], 1); }
}

static void put_opt_int(buf_t *b, opt_int o)
{
    if (o.present) put_int(b, o.v);
    else buf_str(b, "null");
}

/* RFC 8785 / SHALL-008 string serialisation. Input must already be valid UTF-8. */
static void put_string(buf_t *b, const unsigned char *s, size_t n)
{
    static const char hx[] = "0123456789abcdef";
    size_t i;
    buf_put(b, "\"", 1);
    for (i = 0; i < n; i++) {
        unsigned char c = s[i];
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

static void put_cstring(buf_t *b, const char *s)
{
    put_string(b, (const unsigned char *)s, strlen(s));
}

/*
 * Canonical form (SHALL-007). If obs_hash is NULL the member is omitted
 * entirely (the hashed form P of SHALL-010).
 */
static void encode_record(buf_t *b, const record_t *r, const char *input_hash, const char *obs_hash)
{
    buf_str(b, "{\"completion_state\":");
    put_cstring(b, r->completion_state);
    buf_str(b, ",\"failure_type\":");
    if (r->failure_type) put_cstring(b, r->failure_type);
    else buf_str(b, "null");
    buf_str(b, ",\"input_hash\":");
    put_cstring(b, input_hash);
    buf_str(b, ",\"ledger_seq\":");
    put_int(b, r->ledger_seq);
    buf_str(b, ",\"model_id\":");
    put_string(b, r->model_id, r->model_id_len);
    if (obs_hash) {
        buf_str(b, ",\"obs_hash\":");
        put_cstring(b, obs_hash);
    }
    buf_str(b, ",\"oracle_id\":");
    put_string(b, r->oracle_id, r->oracle_id_len);
    buf_str(b, ",\"output\":");
    put_string(b, r->output, r->output_len);
    buf_str(b, ",\"output_size\":");
    put_int(b, r->output_size);
    buf_str(b, ",\"params\":{\"max_tokens\":");
    put_opt_int(b, r->max_tokens);
    buf_str(b, ",\"seed\":");
    put_opt_int(b, r->seed);
    buf_str(b, ",\"temperature\":");
    put_opt_int(b, r->temperature);
    buf_str(b, ",\"top_p\":");
    put_opt_int(b, r->top_p);
    buf_str(b, "}");
    if (r->has_rrs) {
        buf_str(b, ",\"rejected_record_size\":");
        put_int(b, r->rrs);
    }
    buf_str(b, ",\"schema_version\":");
    put_cstring(b, "AX:OBS:v2");
    buf_str(b, "}");
}

/* ------------------------------------------------------------------ */
/* Admission (SHALL-020)                                               */
/* ------------------------------------------------------------------ */

static int state_pair_ok(const record_t *r)
{
    const char *cs = r->completion_state, *ft = r->failure_type;
    if (strcmp(cs, "COMPLETE") == 0 || strcmp(cs, "TRUNCATED") == 0) return ft == NULL;
    if (strcmp(cs, "ERROR") == 0) {
        if (ft == NULL) return 0;
        return strcmp(ft, "TIMEOUT") == 0 || strcmp(ft, "INVALID_OUTPUT") == 0 ||
               strcmp(ft, "TRANSPORT_ERROR") == 0;
    }
    return 0;
}

static int in_range(long long v, long long lo, long long hi) { return v >= lo && v <= hi; }

static int opt_in_range(opt_int o, long long lo, long long hi)
{
    return !o.present || in_range(o.v, lo, hi);
}

static int identifier_ok(const unsigned char *s, size_t n)
{
    size_t i = 0;
    unsigned long cp;
    if (n == 0 || n > 256) return 0;
    if (!utf8_valid(s, n)) return 0;
    while (i < n) {
        int l = utf8_next(s, n, i, &cp);
        if (cp <= 0x1F || (cp >= 0x7F && cp <= 0x9F)) return 0;
        i += (size_t)l;
    }
    return 1;
}

static int output_encoding_ok(const unsigned char *s, size_t n)
{
    size_t i = 0;
    unsigned long cp;
    if (!utf8_valid(s, n)) return 0;
    while (i < n) {
        int l = utf8_next(s, n, i, &cp);
        if ((cp <= 0x1F && cp != 0x0A) || (cp >= 0x7F && cp <= 0x9F)) return 0;
        i += (size_t)l;
    }
    return 1;
}

typedef struct {
    const char *reject;      /* NULL if admitted */
    buf_t stored;            /* stored record when admitted (or measured, if record_size) */
    char obs_hash[65];
} admit_result;

static void admit(const record_t *r, admit_result *res)
{
    char input_hash[65];
    buf_t p = {0, 0, 0};
    sha256_ctx c;
    unsigned char d[32], le[8];
    int i;

    memset(res, 0, sizeof *res);

    /* 1. state_pair */
    if (!state_pair_ok(r)) { res->reject = "state_pair"; return; }

    /* 2. member_not_permitted (SHALL-022). The only non-SHALL-004 member this model can carry is rejected_record_size. */
    if (r->has_rrs) {
        int ok = strcmp(r->completion_state, "ERROR") == 0 && r->failure_type &&
                 strcmp(r->failure_type, "INVALID_OUTPUT") == 0 &&
                 r->output_len == 0 && r->output_size == 0;
        if (!ok) { res->reject = "member_not_permitted"; return; }
    }

    /* 3. integer_range */
    if (!in_range(r->ledger_seq, 0, MAX_SAFE) ||
        !in_range(r->output_size, 0, MAX_SAFE) ||
        !opt_in_range(r->max_tokens, 0, MAX_I32) ||
        !opt_in_range(r->seed, -MAX_SAFE, MAX_SAFE) ||
        !opt_in_range(r->temperature, -MAX_SAFE, MAX_SAFE) ||
        !opt_in_range(r->top_p, -MAX_SAFE, MAX_SAFE) ||
        (r->has_rrs && !in_range(r->rrs, 65537LL, MAX_SAFE))) {
        res->reject = "integer_range";
        return;
    }

    /* 4. identifier */
    if (!identifier_ok(r->model_id, r->model_id_len) || !identifier_ok(r->oracle_id, r->oracle_id_len)) {
        res->reject = "identifier";
        return;
    }

    /* 5. input */
    if (r->input == NULL && r->input_len != 0) { res->reject = "input"; return; }

    /* 6. output_encoding */
    if (!output_encoding_ok(r->output, r->output_len)) { res->reject = "output_encoding"; return; }

    /* 7. output_size (SHALL-019) */
    if (strcmp(r->completion_state, "TRUNCATED") == 0) {
        if (!((unsigned long long)r->output_size > (unsigned long long)r->output_len)) {
            res->reject = "output_size"; return;
        }
    } else if ((unsigned long long)r->output_size != (unsigned long long)r->output_len) {
        res->reject = "output_size"; return;
    }

    /* 8, 9: NFC checks are not implemented (no UCD 16.0.0 data); the harness permits skipping them. */

    /* 10. hashes, stored record, record_size */
    sha256_hex(r->input ? r->input : (const unsigned char *)"", r->input ? r->input_len : 0, input_hash);
    encode_record(&p, r, input_hash, NULL);
    sha256_init(&c);
    sha256_update(&c, (const unsigned char *)"AX:OBS:v2", 9);
    for (i = 0; i < 8; i++) le[i] = (unsigned char)(((unsigned long long)p.len >> (8 * i)) & 0xff);
    sha256_update(&c, le, 8);
    sha256_update(&c, p.p, p.len);
    sha256_final(&c, d);
    to_hex(d, 32, res->obs_hash);
    free(p.p);

    encode_record(&res->stored, r, input_hash, res->obs_hash);
    if (res->stored.len > AX_MAX_OBS_BYTES) {
        res->reject = "record_size";
        res->obs_hash[0] = '\0';
    }
}

/* ------------------------------------------------------------------ */
/* Harness                                                             */
/* ------------------------------------------------------------------ */

static void base(record_t *r)
{
    memset(r, 0, sizeof *r);
    r->completion_state = "COMPLETE";
    r->failure_type = NULL;
    r->input = (const unsigned char *)"prompt"; r->input_len = 6;
    r->ledger_seq = 1;
    r->model_id = (const unsigned char *)"m"; r->model_id_len = 1;
    r->oracle_id = (const unsigned char *)"o"; r->oracle_id_len = 1;
    r->output = (const unsigned char *)"ok"; r->output_len = 2;
    r->output_size = -1; /* sentinel: byte length of output */
}

static void set_out(record_t *r, const char *s) { r->output = (const unsigned char *)s; r->output_len = strlen(s); }

static void finalise(record_t *r)
{
    if (r->output_size == -1) r->output_size = (long long)r->output_len;
}

static void run(const char *name, record_t *r)
{
    admit_result res;
    finalise(r);
    admit(r, &res);
    if (res.reject) {
        printf("REJECT case=%s result=REJECTED(%s)\n", name, res.reject);
    } else {
        char rh[65];
        sha256_hex(res.stored.p, res.stored.len, rh);
        printf("OBS case=%s len=%lu record_sha256=%s obs_hash=%s\n", name,
               (unsigned long)res.stored.len, rh, res.obs_hash);
    }
    free(res.stored.p);
}

static unsigned char *repeat(const char *unit, size_t count)
{
    size_t ul = strlen(unit), i;
    unsigned char *p = (unsigned char *)malloc(ul * count + 1);
    if (!p) { fprintf(stderr, "out of memory\n"); exit(2); }
    for (i = 0; i < count; i++) memcpy(p + i * ul, unit, ul);
    p[ul * count] = 0;
    return p;
}

/* output = '"'*32000 + 'a'*x, otherwise BASE */
static unsigned char *exact_output(size_t x)
{
    unsigned char *p = (unsigned char *)malloc(32000 + x + 1);
    if (!p) { fprintf(stderr, "out of memory\n"); exit(2); }
    memset(p, '"', 32000);
    memset(p + 32000, 'a', x);
    p[32000 + x] = 0;
    return p;
}

static size_t stored_len_for(size_t x, int *admitted)
{
    record_t r;
    admit_result res;
    unsigned char *o = exact_output(x);
    size_t len;
    base(&r);
    r.output = o; r.output_len = 32000 + x;
    finalise(&r);
    admit(&r, &res);
    /* both admitted and record_size results carry the built stored record */
    *admitted = res.reject == NULL;
    len = res.stored.len;
    if (res.reject && strcmp(res.reject, "record_size") != 0) len = 0;
    free(res.stored.p);
    free(o);
    return len;
}

static size_t compute_exact_x(void)
{
    int a0, a1, ax;
    size_t l0 = stored_len_for(0, &a0);
    size_t l1 = stored_len_for(1, &a1);
    size_t slope, x;
    if (l0 == 0 || l1 <= l0) { fprintf(stderr, "exact-limit: non-linear\n"); exit(3); }
    slope = l1 - l0;
    if (l0 > AX_MAX_OBS_BYTES || (AX_MAX_OBS_BYTES - l0) % slope != 0) {
        fprintf(stderr, "exact-limit: no integer solution\n"); exit(3);
    }
    x = (AX_MAX_OBS_BYTES - l0) / slope;
    if (32000 + x < 10000 || 32000 + x > 99999) { fprintf(stderr, "exact-limit: output_size digit count changes\n"); exit(3); }
    if (stored_len_for(x, &ax) != AX_MAX_OBS_BYTES || !ax) { fprintf(stderr, "exact-limit: verification failed\n"); exit(3); }
    return x;
}

int main(void)
{
    record_t r;
    unsigned char *tmp;
    size_t x;

    if (!sha256_selftest()) {
        fprintf(stderr, "SHA-256 self-test failed\n");
        return 1;
    }

    /* Section 1 */
    base(&r); run("base", &r);
    base(&r); r.input = (const unsigned char *)""; r.input_len = 0; run("empty_input", &r);
    base(&r); set_out(&r, "a\nb"); run("lf_output", &r);
    base(&r); set_out(&r, "say \"hi\" \\ done"); run("quote_backslash", &r);
    base(&r); set_out(&r, "caf\xC3\xA9 \xC3\x85 \xF0\x9F\x98\x80"); run("non_ascii", &r);
    base(&r);
    r.max_tokens.present = 1; r.max_tokens.v = 256;
    r.seed.present = 1; r.seed.v = 9007199254740991LL;
    r.temperature.present = 1; r.temperature.v = 45875;
    r.top_p.present = 1; r.top_p.v = 62259;
    run("params_full", &r);
    base(&r);
    r.seed.present = 1; r.seed.v = -9007199254740991LL;
    r.temperature.present = 1; r.temperature.v = -65536;
    r.top_p.present = 1; r.top_p.v = 0;
    run("params_negative", &r);
    base(&r); r.completion_state = "TRUNCATED"; set_out(&r, "abc"); r.output_size = 100000; run("truncated", &r);
    base(&r); r.completion_state = "ERROR"; r.failure_type = "TIMEOUT"; set_out(&r, ""); run("error_timeout", &r);
    base(&r); r.completion_state = "ERROR"; r.failure_type = "INVALID_OUTPUT"; set_out(&r, ""); run("error_invalid_output", &r);
    base(&r); r.completion_state = "ERROR"; r.failure_type = "TRANSPORT_ERROR"; set_out(&r, ""); run("error_transport", &r);
    base(&r); r.ledger_seq = 9007199254740991LL; run("ledger_max", &r);
    base(&r); r.ledger_seq = 0; run("ledger_zero", &r);
    base(&r);
    r.model_id = (const unsigned char *)"mod\xC3\xA8le-1"; r.model_id_len = strlen((const char *)r.model_id);
    r.oracle_id = (const unsigned char *)"\xCF\x89-oracle"; r.oracle_id_len = strlen((const char *)r.oracle_id);
    run("identifiers_utf8", &r);
    tmp = repeat("m", 256);
    base(&r); r.model_id = tmp; r.model_id_len = 256; run("identifier_256", &r);
    free(tmp);
    tmp = repeat("\"", 32000);
    base(&r); r.output = tmp; r.output_len = 32000; run("quote_heavy_fits", &r);
    free(tmp);
    base(&r); set_out(&r, "\xC2\xA0\xE2\x80\xA8\xE2\x80\xA9"); run("del_free_unicode", &r);
    base(&r); r.max_tokens.present = 1; r.max_tokens.v = 0; run("max_tokens_zero", &r);
    base(&r); r.max_tokens.present = 1; r.max_tokens.v = 2147483647LL; run("max_tokens_max", &r);
    base(&r); r.completion_state = "ERROR"; r.failure_type = "INVALID_OUTPUT"; set_out(&r, "");
    r.has_rrs = 1; r.rrs = 70000; run("size_rejection_record", &r);
    x = compute_exact_x();
    tmp = exact_output(x);
    base(&r); r.output = tmp; r.output_len = 32000 + x; run("record_exact_limit", &r);
    free(tmp);

    /* Section 2 */
    base(&r); r.ledger_seq = 9007199254740992LL; run("ledger_over", &r);
    base(&r); r.seed.present = 1; r.seed.v = 9007199254740992LL; run("seed_over", &r);
    base(&r); r.seed.present = 1; r.seed.v = -9007199254740992LL; run("seed_under", &r);
    base(&r); r.completion_state = "TRUNCATED"; r.output_size = 9007199254740992LL; run("output_size_over", &r);
    base(&r); r.failure_type = "TIMEOUT"; run("complete_with_timeout", &r);
    base(&r); r.completion_state = "ERROR"; set_out(&r, ""); run("error_without_failure", &r);
    base(&r); r.completion_state = "TRUNCATED"; r.failure_type = "INVALID_OUTPUT"; run("truncated_with_failure", &r);
    base(&r); r.completion_state = "PARTIAL"; run("unknown_state", &r);
    base(&r); r.model_id = (const unsigned char *)""; r.model_id_len = 0; run("empty_model_id", &r);
    base(&r); r.oracle_id = (const unsigned char *)"o\x01"; r.oracle_id_len = 2; run("control_in_oracle_id", &r);
    base(&r); r.model_id = (const unsigned char *)"m\xC2\x85"; r.model_id_len = 3; run("c1_in_model_id", &r);
    tmp = repeat("m", 257);
    base(&r); r.model_id = tmp; r.model_id_len = 257; run("model_id_257", &r);
    free(tmp);
    base(&r); r.input = NULL; r.input_len = 5; run("null_input", &r);
    base(&r); set_out(&r, "a\x01" "b"); run("control_in_output", &r);
    base(&r); set_out(&r, "a\rb"); run("cr_in_output", &r);
    base(&r); set_out(&r, "x\x7Fy"); run("del_in_output", &r);
    base(&r); set_out(&r, "a\xFF" "b"); run("invalid_utf8_output", &r);
    base(&r); set_out(&r, "ok"); r.output_size = 3; run("size_mismatch", &r);
    base(&r); r.completion_state = "TRUNCATED"; set_out(&r, "abc"); r.output_size = 2; run("truncated_size_under", &r);
    tmp = repeat("\"", 33000);
    base(&r); r.output = tmp; r.output_len = 33000; run("quote_heavy_over", &r);
    free(tmp);
    base(&r); r.failure_type = "TIMEOUT"; r.ledger_seq = 9007199254740992LL; run("order_state_before_range", &r);
    base(&r); r.model_id = (const unsigned char *)""; r.model_id_len = 0; set_out(&r, "a\x01" "b");
    run("order_identifier_before_output", &r);
    base(&r); r.max_tokens.present = 1; r.max_tokens.v = -1; run("max_tokens_negative", &r);
    base(&r); r.max_tokens.present = 1; r.max_tokens.v = 2147483648LL; run("max_tokens_over", &r);
    base(&r); r.has_rrs = 1; r.rrs = 70000; run("rejected_size_on_complete", &r);
    base(&r); r.completion_state = "ERROR"; r.failure_type = "TIMEOUT"; set_out(&r, "");
    r.has_rrs = 1; r.rrs = 70000; run("rejected_size_on_timeout", &r);
    base(&r); r.completion_state = "ERROR"; r.failure_type = "INVALID_OUTPUT"; set_out(&r, "x");
    r.has_rrs = 1; r.rrs = 70000; run("rejected_size_with_output", &r);
    base(&r); r.completion_state = "ERROR"; r.failure_type = "INVALID_OUTPUT"; set_out(&r, "");
    r.has_rrs = 1; r.rrs = 65536; run("rejected_size_too_small", &r);
    tmp = exact_output(x + 1);
    base(&r); r.output = tmp; r.output_len = 32000 + x + 1; run("record_exact_limit_plus_one", &r);
    free(tmp);
    base(&r); r.has_rrs = 1; r.rrs = 70000; r.ledger_seq = 9007199254740992LL; run("order_member_before_range", &r);

    return 0;
}
