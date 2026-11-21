// Testes das contas puras do HUD (rodam no Linux, antes da compilação da DLL): bash build.sh
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include "../src/hud_math.h"

static int g_fail, g_run;
#define CHECK(c) do { g_run++; if (!(c)) { g_fail++; printf("FALHOU %s:%d: %s\n", __FILE__, __LINE__, #c); } } while (0)

static void test_hud_kind(void) {
    CHECK(hud_kind(0x3ff28889dc2f9625ULL) == HUD_SPRITE);    // silhueta do Taz
    CHECK(hud_kind(0x6153c0570df99ae9ULL) == HUD_SPRITE);    // silhueta do Pernalonga
    CHECK(hud_kind(0x16463c6abc682e5cULL) == HUD_BAR_FILL);  // preenchimento da barra
    CHECK(hud_kind(0x6da516a4789678d7ULL) == HUD_BAR_FILL);
    CHECK(hud_kind(0xcdfbd6edf13ed09eULL) == HUD_3D);        // cenoura (modelo 3D)
    CHECK(hud_kind(0x76e79fa1ab5f0475ULL) == HUD_3D);        // claquete (modelo 3D)
    CHECK(hud_kind(0x1234ULL) == HUD_NONE);
}

static void fill(uint32_t *p, int n) { for (int i = 0; i < n; i++) p[i] = 0x01000000u * (uint32_t)(i * 37 % 256) | (uint32_t)(i * 2654435761u & 0xffffff); }

static void test_bmp_roundtrip(void) {
    uint32_t px[5 * 3], back[5 * 3]; fill(px, 15);
    size_t n = bmp32_build(px, 5, 3, NULL);
    CHECK(n == 54 + 5 * 3 * 4);
    unsigned char *buf = malloc(n);
    CHECK(bmp32_build(px, 5, 3, buf) == n);
    int w = 0, h = 0;
    CHECK(bmp32_parse(buf, n, &w, &h, NULL) == 1 && w == 5 && h == 3);
    memset(back, 0, sizeof back);
    CHECK(bmp32_parse(buf, n, &w, &h, back) == 1);
    CHECK(memcmp(px, back, sizeof px) == 0);
    // linha 0 da textura (a de baixo no OpenGL) é a primeira do arquivo (BMP de baixo para cima), em BGRA
    CHECK(buf[54] == (px[0] >> 16 & 0xff) && buf[55] == (px[0] >> 8 & 0xff) && buf[56] == (px[0] & 0xff) && buf[57] == px[0] >> 24);
    free(buf);
}

static void test_bmp_truncated(void) {
    uint32_t px[4 * 4]; fill(px, 16);
    size_t n = bmp32_build(px, 4, 4, NULL);
    unsigned char *buf = malloc(n); bmp32_build(px, 4, 4, buf);
    int w, h;
    CHECK(bmp32_parse(buf, n - 1, &w, &h, NULL) == 0);
    CHECK(bmp32_parse(buf, 20, &w, &h, NULL) == 0);
    free(buf);
}

static void test_bmp_wrong_bpp(void) {
    uint32_t px[4 * 4]; fill(px, 16);
    size_t n = bmp32_build(px, 4, 4, NULL);
    unsigned char *buf = malloc(n); bmp32_build(px, 4, 4, buf);
    buf[28] = 24; buf[29] = 0;                                // biBitCount
    int w, h;
    CHECK(bmp32_parse(buf, n, &w, &h, NULL) == 0);
    free(buf);
}

static void test_rgba_hash_differs(void) {
    uint32_t a[64], b[64]; fill(a, 64); memcpy(b, a, sizeof a);
    CHECK(rgba_hash(a, 64) == rgba_hash(b, 64));
    b[40] ^= 0x00000100u;
    CHECK(rgba_hash(a, 64) != rgba_hash(b, 64));
}

int main(void) {
    test_hud_kind();
    test_bmp_roundtrip();
    test_bmp_truncated();
    test_bmp_wrong_bpp();
    test_rgba_hash_differs();
    printf("%d/%d verificações ok\n", g_run - g_fail, g_run);
    return g_fail ? 1 : 0;
}
