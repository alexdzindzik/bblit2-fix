// Testes dos caracteres da fonte usados para os botões do Xbox nas dicas (rodam no Linux): bash build.sh
#include <stdio.h>
#include "../src/pad_icons.h"

static int g_fail, g_run;
#define CHECK(c) do { g_run++; if (!(c)) { g_fail++; printf("FALHOU %s:%d: %s\n", __FILE__, __LINE__, #c); } } while (0)

// Trecho da tabela caractere -> glifo+1 do bblit2.exe (0x4cf270); 0 = a fonte não desenha nada nesse caractere
static int glyph_of(unsigned char c) {
    switch (c) {
    case 0x0b: return 140; case 0x10: return 142; case 0x12: return 144; case 0x13: return 145;
    case 0xac: return 103; case 0xad: return 104; case 0xae: return 105; case 0xaf: return 106;
    default: return 0;                                   // inclui 0x8c-0x8f
    }
}
// glifo de cada ícone redesenhado (textura do ícone no log menos 61: A 164, X 165, Y 166, B 167, BACK 201, RB 203, LB 205, START 206)
static const int k_icon_glyph[BTN_N] = { 103, 106, 104, 105, 144, 142, 0, 0, 145, 140 };

static void test_icon_chars_hit_their_glyph(void) {
    for (int b = 0; b < BTN_N; b++) {
        unsigned char c = pad_icon_char(b);
        if (!k_icon_glyph[b]) { CHECK(c == 0); continue; }   // LT/RT não têm ícone na fonte: vão pelo nome
        if (glyph_of(c) != k_icon_glyph[b]) printf("  botao %d: caractere %02x -> glifo %d, esperado %d\n", b, c, glyph_of(c), k_icon_glyph[b]);
        CHECK(glyph_of(c) == k_icon_glyph[b]);
    }
}

int main(void) {
    test_icon_chars_hit_their_glyph();
    printf("%d/%d verificações ok\n", g_run - g_fail, g_run);
    return g_fail ? 1 : 0;
}
