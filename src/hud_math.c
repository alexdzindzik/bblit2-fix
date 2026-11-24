// Contas puras do HUD (sem Windows nem OpenGL), testadas em tests/test_hud_math.c
#include "hud_math.h"
#include <stddef.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>

// texturas do HUD da versão PT-BR (hash de desenho, igual em qualquer ajuste de "Brilho")
static const struct { unsigned long long h; int kind; } k_hud[] = {
    { 0x3ff28889dc2f9625ULL, HUD_SPRITE },   // silhueta do Taz + moldura da barra (128x64)
    { 0x6153c0570df99ae9ULL, HUD_SPRITE },   // silhueta do Pernalonga + moldura da barra (256x64)
    { 0x16463c6abc682e5cULL, HUD_BAR_FILL }, // preenchimento das barras de vida (128x8)
    { 0x6da516a4789678d7ULL, HUD_BAR_FILL },
    { 0x8a24a8a3dc058391ULL, HUD_SPRITE }, { 0xdb0a957bf8a45c7eULL, HUD_SPRITE },   // números amarelos 0..9
    { 0x5e0ceaf077778bb8ULL, HUD_SPRITE }, { 0x1cdb8f624aa4c960ULL, HUD_SPRITE },
    { 0x92580ff79274a8f4ULL, HUD_SPRITE }, { 0x593232224dc3f8c3ULL, HUD_SPRITE },
    { 0x08757f739ab6f819ULL, HUD_SPRITE }, { 0x4f1dd2973af8b13eULL, HUD_SPRITE },
    { 0xe72f6db3dc68fc82ULL, HUD_SPRITE }, { 0xf07de88f84ea054aULL, HUD_SPRITE },
    { 0x88d23ca06bc73a6cULL, HUD_SPRITE },   // %
    { 0x81186b738d5b2629ULL, HUD_SPRITE },   // espelho
    { 0xc71b14dfca3365ceULL, HUD_SPRITE },   // espelho riscado
    { 0x7052f27c1a3671b8ULL, HUD_SPRITE },   // X
    { 0xbef4c66e2a96838bULL, HUD_SPRITE },   // mira
    { 0x4c6bdfe92ed18a2dULL, HUD_SPRITE },   // relógio
    { 0x2263ff8efc8e2c56ULL, HUD_SPRITE }, { 0x48b3a3ee49d533c1ULL, HUD_SPRITE },   // medalhas
    { 0x3abd3f8592615d20ULL, HUD_SPRITE }, { 0xd60f246eed512e05ULL, HUD_SPRITE },
    { 0x59f8e33e8c711edeULL, HUD_SPRITE },   // emblema
    { 0x9f99c3526e3e26aeULL, HUD_SPRITE },   // barra laranja
    { 0x1773bbcebaff6d1fULL, HUD_SPRITE }, { 0x814438235e46e7fcULL, HUD_SPRITE },   // setas
    { 0x76e79fa1ab5f0475ULL, HUD_3D },       // claquete (textura de modelo 3D)
    { 0xcdfbd6edf13ed09eULL, HUD_3D },       // cenoura (textura de modelo 3D)
};

int hud_kind(unsigned long long h) {
    for (size_t i = 0; i < sizeof k_hud / sizeof *k_hud; i++)
        if (k_hud[i].h == h) return k_hud[i].kind;
    return HUD_NONE;
}

static unsigned rd16(const unsigned char *p) { return p[0] | p[1] << 8; }
static uint32_t rd32(const unsigned char *p) { return (uint32_t)p[0] | (uint32_t)p[1] << 8 | (uint32_t)p[2] << 16 | (uint32_t)p[3] << 24; }
static void wr16(unsigned char *p, unsigned v) { p[0] = (unsigned char)v; p[1] = (unsigned char)(v >> 8); }
static void wr32(unsigned char *p, uint32_t v) { for (int i = 0; i < 4; i++) p[i] = (unsigned char)(v >> 8 * i); }

int bmp32_parse(const unsigned char *buf, size_t n, int *w, int *h, uint32_t *out) {
    if (!buf || n < 54 || buf[0] != 'B' || buf[1] != 'M') return 0;
    uint32_t off = rd32(buf + 10), hsz = rd32(buf + 14);
    int32_t bw = (int32_t)rd32(buf + 18), bh = (int32_t)rd32(buf + 22);
    unsigned bpp = rd16(buf + 28); uint32_t comp = rd32(buf + 30);
    if (hsz < 40 || bpp != 32 || (comp != 0 && comp != 3)) return 0;
    int top_down = bh < 0; if (top_down) bh = -bh;
    if (bw <= 0 || bh <= 0 || bw > 8192 || bh > 8192) return 0;
    size_t need = (size_t)bw * (size_t)bh * 4;
    if (off < 54 || off > n || n - off < need) return 0;
    *w = bw; *h = bh;
    if (!out) return 1;
    for (int y = 0; y < bh; y++) {
        const unsigned char *src = buf + off + (size_t)(top_down ? bh - 1 - y : y) * bw * 4;
        uint32_t *dst = out + (size_t)y * bw;
        for (int x = 0; x < bw; x++, src += 4)
            dst[x] = (uint32_t)src[2] | (uint32_t)src[1] << 8 | (uint32_t)src[0] << 16 | (uint32_t)src[3] << 24;
    }
    return 1;
}

size_t bmp32_build(const uint32_t *rgba, int w, int h, unsigned char *out) {
    if (w <= 0 || h <= 0) return 0;
    size_t data = (size_t)w * h * 4, total = 54 + data;
    if (!out) return total;
    memset(out, 0, 54);
    out[0] = 'B'; out[1] = 'M'; wr32(out + 2, (uint32_t)total); wr32(out + 10, 54);
    wr32(out + 14, 40); wr32(out + 18, (uint32_t)w); wr32(out + 22, (uint32_t)h);
    wr16(out + 26, 1); wr16(out + 28, 32); wr32(out + 34, (uint32_t)data);
    wr32(out + 38, 2835); wr32(out + 42, 2835);
    unsigned char *d = out + 54;
    for (size_t i = 0; i < (size_t)w * h; i++, d += 4) {
        uint32_t p = rgba[i];
        d[0] = (unsigned char)(p >> 16); d[1] = (unsigned char)(p >> 8); d[2] = (unsigned char)p; d[3] = (unsigned char)(p >> 24);
    }
    return total;
}

unsigned long long rgba_hash(const uint32_t *p, size_t n) {
    unsigned long long h = 1469598103934665603ULL;
    for (size_t i = 0; i < n; i++)
        for (int b = 0; b < 4; b++) { h ^= (p[i] >> 8 * b) & 0xff; h *= 1099511628211ULL; }
    return h;
}

static int gauss_kernel(float sigma, float *k, int kmax) {          // devolve o raio; k[0..2r]
    if (sigma < 0.01f) { k[0] = 1; return 0; }
    int r = (int)ceilf(3 * sigma); if (r > kmax) r = kmax;
    float sum = 0;
    for (int i = -r; i <= r; i++) { k[i + r] = expf(-(float)(i * i) / (2 * sigma * sigma)); sum += k[i + r]; }
    for (int i = 0; i <= 2 * r; i++) k[i] /= sum;
    return r;
}

void hud_shadow_mask(const unsigned char *alpha, int w, int h, int padx, int pady, float sx, float sy, unsigned char *out) {
    int W = w + 2 * padx, H = h + 2 * pady;
    float *a = calloc((size_t)W * H, sizeof *a), *b = calloc((size_t)W * H, sizeof *b);
    float kx[129], ky[129];
    if (!a || !b) { free(a); free(b); memset(out, 0, (size_t)W * H); return; }
    for (int y = 0; y < h; y++) for (int x = 0; x < w; x++) a[(size_t)(y + pady) * W + x + padx] = alpha[(size_t)y * w + x];
    int rx = gauss_kernel(sx, kx, 64), ry = gauss_kernel(sy, ky, 64);
    for (int y = 0; y < H; y++)                                         // horizontal: a -> b
        for (int x = 0; x < W; x++) {
            float v = 0;
            for (int i = -rx; i <= rx; i++) { int xx = x + i; if (xx >= 0 && xx < W) v += a[(size_t)y * W + xx] * kx[i + rx]; }
            b[(size_t)y * W + x] = v;
        }
    for (int y = 0; y < H; y++)                                         // vertical: b -> saída
        for (int x = 0; x < W; x++) {
            float v = 0;
            for (int i = -ry; i <= ry; i++) { int yy = y + i; if (yy >= 0 && yy < H) v += b[(size_t)yy * W + x] * ky[i + ry]; }
            int q = (int)(v + 0.5f);
            out[(size_t)y * W + x] = (unsigned char)(v > 1e-6f && q < 1 ? 1 : q > 255 ? 255 : q);
        }
    free(a); free(b);
}

int hud_fonte_action(int bmp_exists, int txt_ok, unsigned long long txt_hash, unsigned long long cur_hash) {
    if (!bmp_exists || !txt_ok) return FONTE_WRITE;
    return txt_hash == cur_hash ? FONTE_KEEP : FONTE_REWRITE;
}
