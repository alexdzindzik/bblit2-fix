// Testes do ajuste de cor do menu Vídeo (rodam no Linux): bash build.sh
#include <math.h>
#include <stdio.h>
#include "../src/coloradj.h"

static int g_fail, g_run;
#define CHECK(c) do { g_run++; if (!(c)) { g_fail++; printf("FALHOU %s:%d: %s\n", __FILE__, __LINE__, #c); } } while (0)
#define NEAR(a, b) (fabs((double)(a) - (double)(b)) < 1e-4)

static void test_rgb2hsi(void) {
    double h, s, i;
    ca_rgb2hsi(1, 0, 0, &h, &s, &i); CHECK(NEAR(h, 0) && NEAR(s, 1) && NEAR(i, 1.0 / 3));
    ca_rgb2hsi(0, 1, 0, &h, &s, &i); CHECK(NEAR(h, 120) && NEAR(s, 1));
    ca_rgb2hsi(0, 0, 1, &h, &s, &i); CHECK(NEAR(h, 240) && NEAR(s, 1));
    ca_rgb2hsi(.5, .5, .5, &h, &s, &i); CHECK(h == 0 && s == 0 && NEAR(i, .5));
}
static void test_roundtrip(void) {
    static const double c[][3] = { { .8, .4, .2 }, { .1, .7, .3 }, { .3, .2, .9 }, { .9, .9, .1 }, { .6, .1, .6 } };
    for (int k = 0; k < 5; k++) {
        double h, s, i, r, g, b;
        ca_rgb2hsi(c[k][0], c[k][1], c[k][2], &h, &s, &i);
        ca_hsi2rgb(h, s, i, &r, &g, &b);
        CHECK(NEAR(r, c[k][0]) && NEAR(g, c[k][1]) && NEAR(b, c[k][2]));
    }
}
static void test_fix_wrap(void) {   // 0x407d30 grava só o byte baixo de cor·255 truncado: 382 vira 126
    unsigned char red[3] = { 255, 0, 0 }, g1[3] = { 126, 0, 0 };            // vermelho com intensidade ·1,5: r = 1,5
    CHECK(ca_fix_wrap(red, g1, 0, 1, 1.5) == 1 && g1[0] == 255 && g1[1] == 0 && g1[2] == 0);
    unsigned char c[3] = { 100, 50, 25 }, g2[3] = { 120, 60, 30 };           // sem estouro: fica o que o jogo gravou
    CHECK(ca_fix_wrap(c, g2, 0, 1, 1.2) == 0 && g2[0] == 120 && g2[1] == 60 && g2[2] == 30);
    unsigned char w[3] = { 250, 250, 250 }, g3[3] = { 250, 250, 250 };       // cinza: I no máximo 1, sem estouro
    CHECK(ca_fix_wrap(w, g3, 0, 1, 2) == 0 && g3[0] == 250);
}
// cor por vértice (partes sem textura): o mesmo ajuste que a paleta recebe
static void test_adjust(void) {
    float o[3];
    const float pele[3] = { .8f, .6f, .45f };
    ca_adjust(pele, o, 0, 1, 1);                                              // neutro: igual, sem conta
    CHECK(o[0] == pele[0] && o[1] == pele[1] && o[2] == pele[2]);
    ca_adjust(pele, o, 0, 1, 1.125);                                          // Brilho padrão: I · 1,125, mesma cor
    CHECK(NEAR(o[0], .8 * 1.125) && NEAR(o[1], .6 * 1.125) && NEAR(o[2], .45 * 1.125));
    const float cinza[3] = { .5f, .5f, .5f };
    ca_adjust(cinza, o, 90, 2, 1);                                            // cinza: matiz e saturação não mudam nada
    CHECK(NEAR(o[0], .5) && NEAR(o[1], .5) && NEAR(o[2], .5));
    const float claro[3] = { 1, .8f, .2f };
    ca_adjust(claro, o, 0, 1, 1.67);                                          // estouro: limitado a 1, como a paleta
    CHECK(o[0] == 1 && o[1] <= 1 && o[2] >= 0 && o[1] > .8f);
    // matiz, saturação e brilho juntos: no modelo HSI do jogo, H + 10, S · 1,2 e I · 1,125, como na paleta
    const float cf[3] = { 180 / 255.f, 140 / 255.f, 110 / 255.f };
    ca_adjust(cf, o, 10, 1.2, 1.125);
    double h0, s0, i0, h1, s1, i1;
    ca_rgb2hsi(cf[0], cf[1], cf[2], &h0, &s0, &i0); ca_rgb2hsi(o[0], o[1], o[2], &h1, &s1, &i1);
    CHECK(NEAR(h1, h0 + 10) && NEAR(s1, s0 * 1.2) && NEAR(i1, i0 * 1.125));
}

int main(void) {
    test_rgb2hsi(); test_roundtrip(); test_fix_wrap(); test_adjust();
    printf("%d/%d verificações ok\n", g_run - g_fail, g_run);
    return g_fail != 0;
}
