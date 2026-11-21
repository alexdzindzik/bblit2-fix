#include "pad_icons.h"
// ✕ □ △ ○ do PS1 ficam em 0xac-0xaf na tabela de glifos do jogo (0x4cf270: glifos 103-106); R1 0x10, L1 0x12,
// Esc 0x13 e "START" 0x0b. LT e RT não têm símbolo na fonte.
static const unsigned char k_char[BTN_N] = { 0xac, 0xaf, 0xad, 0xae, 0x12, 0x10, 0, 0, 0x13, 0x0b };
unsigned char pad_icon_char(int btn) { return btn >= 0 && btn < BTN_N ? k_char[btn] : 0; }
// Quadros em que o jogo desenha esses glifos na tela (2560x1440; medidos nos anéis das capturas e com o F10):
// ícones do PS1 32x32 -> ~72,8x98 px, Enter/Esc 32x16 -> 101x90 px, "START" 64x16 -> 176x90 px, todos a partir do
// topo das letras. As letras (16x16 -> 56x90) têm o miolo de 40 px centrado a 60 px do topo. Todo botão sai
// redondo na tela, com 60 px de diâmetro e centrado na altura das letras.
#define ICON_D 60.0f
#define LETTER_MID_PX 60.0f
static void round_geom(ICON_GEOM *g, int w, int h, float qw, float qh, float cx, float cy_max) {
    float pc = qw / w, pr = qh / h;
    g->ay = pr / pc;
    g->rad = ICON_D / 2 / pc;
    g->cx = cx < 0 ? g->rad + 0.8f : cx;                  // cx < 0: encostado à esquerda (com 0,8 coluna de folga)
    g->cy = LETTER_MID_PX / pr;
    if (g->cy > cy_max) g->cy = cy_max;
}
int pad_icon_geom(int btn, int w, int h, ICON_GEOM *g) {
    if (btn >= BTN_A && btn <= BTN_Y && w == 32 && h == 32) { round_geom(g, w, h, 72.8f, 98, -1, 32); return 1; }
    if ((btn == BTN_A || btn == BTN_B || btn == BTN_START) && w == 32 && h == 16) { round_geom(g, w, h, 101, 90, 16, 10.6f); return 1; }
    if (btn == BTN_BACK && w == 64 && h == 16) { round_geom(g, w, h, 176, 90, -1, 10.6f); return 1; }
    return 0;
}
// o "START" do PS1 (0x0b) avança 50; com o ícone redondo do BACK no lugar, avança como os outros ícones
int pad_icon_advance(int btn) { return btn == BTN_BACK ? 17 : 0; }
int pad_menu_back(unsigned now, unsigned prompt_tick) {
    return prompt_tick != 0 && (unsigned)(now - prompt_tick) < 300u;
}
