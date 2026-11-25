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

int main(void) {
    test_rgb2hsi(); test_roundtrip(); test_fix_wrap();
    printf("%d/%d verificações ok\n", g_run - g_fail, g_run);
    return g_fail != 0;
}
