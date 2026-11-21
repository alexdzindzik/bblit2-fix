// Testes da centralização dos menus nome/valor (rodam no Linux): bash build.sh
#include <stdio.h>
#include <string.h>
#include "../src/menu_center.h"

static int g_fail, g_run;
#define CHECK(c) do { g_run++; if (!(c)) { g_fail++; printf("FALHOU %s:%d: %s\n", __FILE__, __LINE__, #c); } } while (0)

static unsigned char k_adv[256];
static void font(void) { memset(k_adv, 10, sizeof k_adv); k_adv[' '] = 0; }   // toda letra avança 10
// monta um texto no formato do jogo
static int op(unsigned char *b, int o, int code, const unsigned char *d, int n) {
    b[o] = (unsigned char)code; b[o + 1] = (unsigned char)((n + 3) >> 8); b[o + 2] = (unsigned char)(n + 3);
    if (n) memcpy(b + o + 3, d, n);
    return o + 3 + n;
}
static int op16(unsigned char *b, int o, int code, int v) { unsigned char d[2] = { (unsigned char)(v >> 8), (unsigned char)v }; return op(b, o, code, d, 2); }
static int line(unsigned char *b, int o, int align, int x, int y, const char *t) {
    unsigned char d[64]; d[0] = 0x15; int n = (int)strlen(t); memcpy(d + 1, t, n);
    o = op(b, o, 0x01, NULL, 0); o = op16(b, o, align, x); o = op16(b, o, 0x04, y);
    unsigned char c = 3; o = op(b, o, 0x05, &c, 1);
    return op(b, o, 0x0a, d, n + 1);
}
static int end(unsigned char *b, int o) { b[o] = 0x18; b[o + 1] = 0; b[o + 2] = 0; return o + 3; }

static void test_scan_reads_x_y_width(void) {
    unsigned char b[256]; int o = op(b, 0, 0x00, NULL, 0);
    o = line(b, o, MC_RIGHT, 10, -30, "abc de"); o = end(b, o);
    MC_RUN r[4]; int n = mc_scan(b, o, k_adv, 1.0f, r, 4);
    CHECK(n == 1);
    CHECK(r[0].align == MC_RIGHT && r[0].x == 10 && r[0].y == -30);
    CHECK(r[0].w > 55.9f && r[0].w < 56.1f);             // 5 letras x 10 + espaço 6
    CHECK(b[r[0].xoff] == 0 && b[r[0].xoff + 1] == 10);  // aponta os bytes do x
}
static void test_scan_stops_at_end_and_bad_sizes(void) {
    unsigned char b[64]; int o = end(b, 0); o = line(b, o, MC_LEFT, 1, 2, "x");
    MC_RUN r[4];
    CHECK(mc_scan(b, o, k_adv, 1.0f, r, 4) == 0);        // nada depois do 0x18
    unsigned char bad[8] = { 0x0a, 0x00, 0x01, 0, 0, 0, 0, 0 };
    CHECK(mc_scan(bad, 8, k_adv, 1.0f, r, 4) == 0);      // tamanho menor que o cabeçalho
}
// o menu de som: nomes à direita em x=10 (25 no primeiro), valores à esquerda em x=20 (35)
static int sound_menu(MC_RUN *r) {
    unsigned char a[512], v[4][128]; int n = 0, o;
    o = op(a, 0, 0x00, NULL, 0);
    o = line(a, o, MC_RIGHT, 25, -45, "configuracao altofalante");
    o = line(a, o, MC_RIGHT, 10, -30, "volume da musica");
    o = line(a, o, MC_RIGHT, 10, -15, "volume do som");
    o = line(a, o, MC_RIGHT, 10, 0, "barra de energia"); o = end(a, o);
    n += mc_scan(a, o, k_adv, 1.0f, r + n, 32 - n);
    const char *val[4] = { "estereo", "iiiii", "iiiii", "nao" }; int vx[4] = { 35, 20, 20, 20 }, vy[4] = { -45, -30, -15, 0 };
    for (int i = 0; i < 4; i++) { o = op(v[i], 0, 0x00, NULL, 0); o = line(v[i], o, MC_LEFT, vx[i], vy[i], val[i]); o = end(v[i], o); n += mc_scan(v[i], o, k_adv, 1.0f, r + n, 32 - n); }
    return n;
}
static void test_sound_menu_block_is_centered(void) {
    MC_RUN r[32]; unsigned char use[32]; int n = sound_menu(r);
    int s = mc_shift(r, n, use);
    float left = 1e9f, right = -1e9f;
    for (int i = 0; i < n; i++) {
        CHECK(use[i]);                                   // todas as 8 linhas fazem parte do menu
        float x = (float)(r[i].x + s), a = r[i].align == MC_RIGHT ? x - r[i].w : x, b = r[i].align == MC_RIGHT ? x : x + r[i].w;
        if (a < left) left = a;
        if (b > right) right = b;
    }
    CHECK(s > 0);                                        // os nomes longos empurravam para a esquerda
    CHECK(left + right > -2 && left + right < 2);        // bloco centrado no meio da tela (x = 0)
}
static void test_lone_texts_are_left_alone(void) {
    unsigned char b[256]; int o = op(b, 0, 0x00, NULL, 0);
    o = line(b, o, MC_RIGHT, 10, -30, "so um nome"); o = line(b, o, MC_LEFT, 20, 90, "valor noutra linha"); o = end(b, o);
    MC_RUN r[4]; unsigned char use[4]; int n = mc_scan(b, o, k_adv, 1.0f, r, 4);
    CHECK(mc_shift(r, n, use) == 0);                     // sem 2 linhas nome+valor: não é um menu desses
    CHECK(!use[0] && !use[1]);
    unsigned char c[128]; o = op(c, 0, 0x00, NULL, 0); o = line(c, o, MC_CENTER, 0, 75, "selecionar"); o = end(c, o);
    n = mc_scan(c, o, k_adv, 1.0f, r, 4);
    CHECK(mc_shift(r, n, use) == 0 && !use[0]);          // textos centrados (dicas, títulos) não mexem
}

int main(void) {
    font();
    test_scan_reads_x_y_width();
    test_scan_stops_at_end_and_bad_sizes();
    test_sound_menu_block_is_centered();
    test_lone_texts_are_left_alone();
    printf("%d/%d verificações ok\n", g_run - g_fail, g_run);
    return g_fail ? 1 : 0;
}
