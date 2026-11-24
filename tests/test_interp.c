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

static void test_room_point(void) {   // 0x408990: vista = linhas m[0..8] · v + m[9..11]; projeção pela vista z
    float m[12] = { 1, 0, 0,  0, 1, 0,  0, 0, 1,  0, 0, -100 }, o[3];
    short v[3] = { 10, 20, 0 };
    CHECK(ip_room_point(m, 500, -0.5f, v, o) && NEAR(o[0], 50) && NEAR(o[1], 100) && NEAR(o[2], -100));   // z < 0: ×K/−z
    m[11] = 5; short w[3] = { 1, 2, 0 };
    CHECK(ip_room_point(m, 500, -0.5f, w, o) && NEAR(o[0], 2) && NEAR(o[1], 4) && o[2] == -0.5f);         // z > 0: ×2, sentinela
    m[11] = 0; short u[3] = { 3, 4, 0 };
    CHECK(ip_room_point(m, 500, -0.5f, u, o) && NEAR(o[0], 1500) && NEAR(o[1], 2000) && o[2] == 1.0f);    // z = 0: ×K, 1,0
    float r[12] = { 0, 0, 1,  0, 1, 0,  -1, 0, 0,  0, 0, -50 };   // giro de 90°: x de vista = z, z de vista = −x − 50
    short s[3] = { 10, 0, 20 };
    CHECK(ip_room_point(r, 600, -0.5f, s, o) && NEAR(o[0], 200) && NEAR(o[1], 0) && NEAR(o[2], -60));
    short big[3] = { 32001, 0, 0 }, edge[3] = { 32000, -32000, 0 }, bigy[3] = { 0, -32001, 0 };
    CHECK(!ip_room_point(m, 500, -0.5f, big, o) && !ip_room_point(m, 500, -0.5f, bigy, o));               // fora de ±32000: invisível
    CHECK(ip_room_point(m, 500, -0.5f, edge, o));
}

int main(void) {
    test_div(); test_vertex_ends(); test_vertex_same_z(); test_vertex_perspective(); test_dist(); test_texcoord(); test_map(); test_keys(); test_behind();
    test_room_point();
    printf("%d/%d verificações ok\n", g_run - g_fail, g_run);
    return g_fail != 0;
}
