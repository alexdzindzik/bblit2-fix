// Testes do instantâneo de comandos OpenGL (rodam no Linux): bash build.sh
#include <stdio.h>
#include <string.h>
#include "../src/glrec.h"

static int g_fail, g_run;
#define CHECK(c) do { g_run++; if (!(c)) { g_fail++; printf("FALHOU %s:%d: %s\n", __FILE__, __LINE__, #c); } } while (0)

static GR_CMD g_got[8];
static int g_ngot;
static void collect(const GR_CMD *c, void *ctx) {
    (void)ctx;
    if (g_ngot < 8) g_got[g_ngot] = *c;
    g_ngot++;
}

static void test_put_replay_order(void) {
    GR_CMD buf[8]; GR_SNAP s; gr_init(&s, buf, 8);
    unsigned mode = 4, ptr = 0x5200010;
    double v[3] = { 1.5, -2.25, 300 };
    CHECK(gr_put(&s, GR_BEGIN, &mode, 1, NULL, 0) == 1);
    CHECK(gr_put(&s, GR_VERTEX, &ptr, 1, v, 3) == 1);
    CHECK(gr_put(&s, GR_END, NULL, 0, NULL, 0) == 1);
    g_ngot = 0;
    CHECK(gr_replay(&s, collect, NULL) == 3);
    CHECK(g_ngot == 3);
    CHECK(g_got[0].op == GR_BEGIN && g_got[0].u[0] == 4);
    CHECK(g_got[1].op == GR_VERTEX && g_got[1].u[0] == 0x5200010);
    CHECK(g_got[1].d[0] == 1.5 && g_got[1].d[1] == -2.25 && g_got[1].d[2] == 300);
    CHECK(g_got[1].u[1] == 0 && g_got[1].d[3] == 0);                 // o que não foi passado fica 0
    CHECK(g_got[2].op == GR_END);
}
static void test_float_exact(void) {
    GR_CMD buf[2]; GR_SNAP s; gr_init(&s, buf, 2);
    float in[3] = { -1280.0f, 0.018f, 65536.0f };
    double d[3] = { in[0], in[1], in[2] };
    gr_put(&s, GR_TEXCOORD, NULL, 0, d, 3);
    g_ngot = 0; gr_replay(&s, collect, NULL);
    CHECK((float)g_got[0].d[0] == in[0] && (float)g_got[0].d[1] == in[1] && (float)g_got[0].d[2] == in[2]);
}
static void test_overflow(void) {
    GR_CMD buf[2]; GR_SNAP s; gr_init(&s, buf, 2);
    CHECK(gr_put(&s, GR_END, NULL, 0, NULL, 0) == 1);
    CHECK(gr_put(&s, GR_END, NULL, 0, NULL, 0) == 1);
    CHECK(gr_put(&s, GR_END, NULL, 0, NULL, 0) == 0);
    CHECK(s.overflow == 1 && s.n == 2);
    g_ngot = 0;
    CHECK(gr_replay(&s, collect, NULL) == 0);
    CHECK(g_ngot == 0);                                               // instantâneo incompleto não é repetido
}
static void test_reset(void) {
    GR_CMD buf[1]; GR_SNAP s; gr_init(&s, buf, 1);
    gr_put(&s, GR_END, NULL, 0, NULL, 0); gr_put(&s, GR_END, NULL, 0, NULL, 0);
    gr_reset(&s);
    CHECK(s.n == 0 && s.overflow == 0);
    unsigned m = 7;
    CHECK(gr_put(&s, GR_MATRIXMODE, &m, 1, NULL, 0) == 1);
    g_ngot = 0;
    CHECK(gr_replay(&s, collect, NULL) == 1 && g_got[0].u[0] == 7);
}
static void test_raw_flag(void) {
    GR_CMD buf[1]; GR_SNAP s; gr_init(&s, buf, 1);
    unsigned cap = 0x0BE2;
    gr_put(&s, GR_ENABLE | GR_RAW, &cap, 1, NULL, 0);
    g_ngot = 0; gr_replay(&s, collect, NULL);
    CHECK(g_got[0].op == (GR_ENABLE | GR_RAW) && (g_got[0].op & ~GR_RAW) == GR_ENABLE && g_got[0].u[0] == 0x0BE2);
}

int main(void) {
    test_put_replay_order();
    test_float_exact();
    test_overflow();
    test_reset();
    test_raw_flag();
    printf("%d/%d verificações ok\n", g_run - g_fail, g_run);
    return g_fail != 0;
}
