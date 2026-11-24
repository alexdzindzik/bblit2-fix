// Testes dos horários dos quadros extras (rodam no Linux): bash build.sh
#include <stdio.h>
#include "../src/pacer.h"

static int g_fail, g_run;
#define CHECK(c) do { g_run++; if (!(c)) { g_fail++; printf("FALHOU %s:%d: %s\n", __FILE__, __LINE__, #c); } } while (0)

static const long long F = 10000000, TPF = 10000000 / 30, M = 20000;   // QPC de 10 MHz, 2 ms de margem

static int extras_in_tick(long long g0, long long period, long long start) {   // quantos quadros extras num tick
    int n = 0; long long after = start, t;
    while ((t = pacer_next(g0, period, start, after, start + TPF, M)) >= 0) { n++; after = t; }
    return n;
}

static void test_period(void) {
    CHECK(pacer_period(F, 60) == 166667);
    CHECK(pacer_period(F, 120) == 83333);
    CHECK(pacer_period(F, 144) == 69444);
}
static void test_60_one_extra(void) {
    long long g0 = 1000000, p = pacer_period(F, 60);
    CHECK(pacer_next(g0, p, g0, g0, g0 + TPF, M) == g0 + 166667);
    CHECK(pacer_next(g0, p, g0, g0 + 166667, g0 + TPF, M) == -1);
    CHECK(extras_in_tick(g0, p, g0) == 1);
}
static void test_120_three_extra(void) {
    long long g0 = 1000000, p = pacer_period(F, 120);
    CHECK(extras_in_tick(g0, p, g0) == 3);
    CHECK(extras_in_tick(g0, p, g0 + 7 * TPF) == 3);
}
static void test_144_average(void) {
    long long g0 = 1000000, p = pacer_period(F, 144);
    int total = 0;
    for (int k = 0; k < 300; k++) total += 1 + extras_in_tick(g0, p, g0 + k * TPF);   // 1 = o quadro do jogo
    CHECK(total >= 1437 && total <= 1443);                                           // 10 s a 144 por segundo
}
static void test_margin(void) {
    long long g0 = 1000000, p = pacer_period(F, 60);
    CHECK(pacer_next(g0, p, g0, g0, g0 + TPF, 200000) == -1);    // margem grande: o ponto cairia perto demais do fim
}
static void test_late(void) {
    long long g0 = 1000000, p = pacer_period(F, 60);
    CHECK(pacer_next(g0, p, g0, g0 + TPF + 5, g0 + TPF, M) == -1);   // já passou do fim do tick
}

int main(void) {
    test_period();
    test_60_one_extra();
    test_120_three_extra();
    test_144_average();
    test_margin();
    test_late();
    printf("%d/%d verificações ok\n", g_run - g_fail, g_run);
    return g_fail != 0;
}
