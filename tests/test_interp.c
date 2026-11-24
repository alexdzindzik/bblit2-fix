// Testes da interpolação entre ticks (rodam no Linux): bash build.sh
#include <math.h>
#include <stdio.h>
#include "../src/interp.h"

static int g_fail, g_run;
#define CHECK(c) do { g_run++; if (!(c)) { g_fail++; printf("FALHOU %s:%d: %s\n", __FILE__, __LINE__, #c); } } while (0)
#define NEAR(a, b) (fabs((double)(a) - (double)(b)) < 1e-3)

static void test_div(void) {
    CHECK(ip_div(0) == 1); CHECK(ip_div(9) == 10); CHECK(ip_div(-4) == 4);
}
static void test_vertex_ends(void) {
    float a[3] = { 100, -50, 9 }, b[3] = { 140, 20, 30 }, o[3];
    ip_vertex(a, b, 0, o); CHECK(NEAR(o[0], a[0]) && NEAR(o[1], a[1]) && NEAR(o[2], a[2]));
    ip_vertex(a, b, 1, o); CHECK(NEAR(o[0], b[0]) && NEAR(o[1], b[1]) && NEAR(o[2], b[2]));
}
static void test_vertex_same_z(void) {
    float a[3] = { 100, -40, 9 }, b[3] = { 200, 40, 9 }, o[3];
    ip_vertex(a, b, 0.5, o); CHECK(NEAR(o[0], 150) && NEAR(o[1], 0) && NEAR(o[2], 9));
}
static void test_vertex_perspective(void) {   // x de vista 1000 -> 2000 indo para longe: no meio, x 1500, z 14
    float a[3] = { 100, 0, 9 }, b[3] = { 100, 0, 19 }, o[3];
    ip_vertex(a, b, 0.5, o); CHECK(NEAR(o[2], 14) && NEAR(o[0], 100));
    float c[3] = { 50, 0, -3 }, d[3] = { 50, 0, -3 };  // atrás da câmera: D = −z
    ip_vertex(c, d, 0.5, o); CHECK(NEAR(o[0], 50) && NEAR(o[2], -3));
}
static void test_dist(void) {
    float a[3] = { 0, 0, 9 }, b[3] = { 0, 0, 19 }, c[3] = { 10, 0, 9 };
    CHECK(NEAR(ip_dist(a, b), 10));
    CHECK(NEAR(ip_dist(c, a), 100));
}
static void test_texcoord(void) {
    double tc[4] = { 2 * 0.1, 4 * 0.1, 0, 0.1 }, o[4];
    ip_texcoord(tc, 20, o); CHECK(NEAR(o[0], 0.1) && NEAR(o[1], 0.2) && o[2] == 0 && NEAR(o[3], 0.05));
    ip_texcoord(tc, -20, o); CHECK(NEAR(o[3], 0.05));                     // q = 1/|z|
    double z0[4] = { 0.3, 0.6, 0, 0 };
    ip_texcoord(z0, 20, o); CHECK(o[0] == 0.3 && o[1] == 0.6 && o[3] == 0); // q = 0: copia
}
static void test_map(void) {
    unsigned k0[8], k1[8]; int v[8]; IP_MAP m;
    ip_map_init(&m, k0, k1, v, 8);
    CHECK(ip_map_put(&m, 0x05365040, 1, 10) && ip_map_put(&m, 0x05365040, 2, 20) && ip_map_put(&m, 0x42, 1, 30));
    CHECK(ip_map_get(&m, 0x05365040, 1) == 10 && ip_map_get(&m, 0x05365040, 2) == 20 && ip_map_get(&m, 0x42, 1) == 30);
    CHECK(ip_map_get(&m, 0x42, 2) == -1);
    CHECK(ip_map_put(&m, 0x42, 1, 31) && ip_map_get(&m, 0x42, 1) == 31);   // a mesma chave troca o valor
    ip_map_clear(&m); CHECK(ip_map_get(&m, 0x05365040, 1) == -1 && m.n == 0);
    unsigned a0[4], a1[4]; int av[4]; IP_MAP s; ip_map_init(&s, a0, a1, av, 4);
    int ok = 1; for (unsigned i = 1; i <= 3; i++) ok &= ip_map_put(&s, i, i, (int)i);
    CHECK(ok && ip_map_put(&s, 9, 9, 9) == 0);                             // cheio (deixa uma vaga para o fim da busca)
}

static void test_keys(void) {
    unsigned k = ip_key1(1, 300, 7);
    CHECK(ip_key_obj(k) == 1);
    CHECK(k != ip_key1(1, 301, 7) && k != ip_key1(0, 300, 7) && k != ip_key1(1, 300, 8));   // ocorrência, n e ponto contam
    CHECK(ip_key1(0, 0, 0) == 0 && ip_key_obj(ip_key1(15, 16383, 16383)) == 15);
}
static void test_behind(void) {
    CHECK(!ip_behind(-1500, -0.5f));
    CHECK(ip_behind(10, -0.5f));        // atrás da câmera
    CHECK(ip_behind(-0.5f, -0.5f));     // valor que o cenário grava para z > 0
    CHECK(ip_behind(1.0f, -0.5f));      // valor que o cenário grava para z = 0
    CHECK(ip_behind(-1.5f, -1000));     // perto demais do plano da câmera
}

static void test_widen(void) {   // limites de tela do jogo { x mín, x máx, y mín, y máx }, alargados em volta do centro
    float b[4] = { -1280, 1279, -719, 720 }, o[4];
    ip_widen(b, 2, 1.5f, o);
    CHECK(NEAR(o[0], -2559.5) && NEAR(o[1], 2558.5) && NEAR(o[2], -1078.75) && NEAR(o[3], 1079.75));
    ip_widen(b, 1, 1, o);
    CHECK(o[0] == b[0] && o[1] == b[1] && o[2] == b[2] && o[3] == b[3]);
}

int main(void) {
    test_div(); test_vertex_ends(); test_vertex_same_z(); test_vertex_perspective(); test_dist(); test_texcoord(); test_map(); test_keys(); test_behind(); test_widen();
    printf("%d/%d verificações ok\n", g_run - g_fail, g_run);
    return g_fail != 0;
}
