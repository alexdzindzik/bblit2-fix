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

// Quadro em que o jogo desenha cada glifo (pixels numa tela 2560x1440, medidos com o F10 e nos anéis das capturas):
// letra 16x16 -> 56x90, ícone do PS1 32x32 -> ~73x98, Enter/Esc 32x16 -> 101x90, "START" 64x16 -> 176x90. Todos
// começam no topo das letras; o miolo das maiúsculas ocupa 40 px de altura, centrado a 60 px do topo.
#define LETTER_H 40.0f
#define LETTER_MID 60.0f
static void check_round_button(int btn, int w, int h, float qw, float qh) {
    ICON_GEOM g;
    CHECK(pad_icon_geom(btn, w, h, &g));
    float pc = qw / w, pr = qh / h;                      // pixels por coluna e por linha da textura
    float ry = g.rad / g.ay;                             // raio em linhas
    CHECK(g.cx - g.rad >= 0 && g.cx + g.rad <= w);       // cabe na textura
    CHECK(g.cy - ry >= 0 && g.cy + ry <= h);
    float dw = 2 * g.rad * pc, dh = 2 * ry * pr;         // tamanho na tela
    CHECK(dw / dh > 0.96f && dw / dh < 1.04f);           // redondo na tela
    CHECK(dw > 58 && dw < 62);                           // todos do mesmo tamanho (60 px, 1,5x a altura das letras)
    float mid = g.cy * pr;
    CHECK(mid > LETTER_MID - 6 && mid < LETTER_MID + 6); // centrado na altura das letras
    if (!(dw / dh > 0.96f && dw / dh < 1.04f) || !(mid > LETTER_MID - 6 && mid < LETTER_MID + 6) || !(dw > 58 && dw < 62))
        printf("  botao %d: %.0fx%.0f px, centro a %.0f px do topo\n", btn, dw, dh, mid);
}
static void test_face_buttons_round_and_letter_sized(void) {
    for (int b = BTN_A; b <= BTN_Y; b++) check_round_button(b, 32, 32, 72.8f, 98);
}
static void test_enter_and_menu_match_the_face_buttons(void) {
    check_round_button(BTN_A, 32, 16, 101, 90);          // tecla Enter -> Ⓐ (ou Ⓑ com swap_ab)
    check_round_button(BTN_B, 32, 16, 101, 90);
    check_round_button(BTN_START, 32, 16, 101, 90);      // tecla Esc -> Menu
}
static void test_back_is_a_round_button(void) {
    check_round_button(BTN_BACK, 64, 16, 176, 90);
}
static void test_back_advance_matches_icon(void) {
    CHECK(pad_icon_advance(BTN_BACK) == 17);             // o "START" do PS1 avançava 50; o ícone redondo ocupa ~17
    CHECK(pad_icon_advance(BTN_A) == 0);                 // os demais ficam como o jogo definiu
}

static void test_b_goes_back_while_f4_prompt_on_screen(void) {
    CHECK(pad_menu_back(10000, 9900) == 1);              // a dica "F4 menu anterior" passou há 100 ms
    CHECK(pad_menu_back(10000, 9000) == 0);              // faz 1 s que ela não aparece: B volta a ser só "orelhas"
    CHECK(pad_menu_back(10000, 0) == 0);                 // nunca apareceu
    CHECK(pad_menu_back(50, 0xFFFFFFF0u) == 1);          // GetTickCount deu a volta
}

int main(void) {
    test_icon_chars_hit_their_glyph();
    test_face_buttons_round_and_letter_sized();
    test_enter_and_menu_match_the_face_buttons();
    test_back_is_a_round_button();
    test_back_advance_matches_icon();
    test_b_goes_back_while_f4_prompt_on_screen();
    printf("%d/%d verificações ok\n", g_run - g_fail, g_run);
    return g_fail ? 1 : 0;
}
