#include "pad_icons.h"
// ✕ □ △ ○ do PS1 ficam em 0xac-0xaf na tabela de glifos do jogo (0x4cf270: glifos 103-106); R1 0x10, L1 0x12,
// Esc 0x13 e "START" 0x0b. LT e RT não têm símbolo na fonte.
static const unsigned char k_char[BTN_N] = { 0xac, 0xaf, 0xad, 0xae, 0x12, 0x10, 0, 0, 0x13, 0x0b };
unsigned char pad_icon_char(int btn) { return btn >= 0 && btn < BTN_N ? k_char[btn] : 0; }
// Quadros em que o jogo desenha esses glifos na tela (medidos): ícones do PS1 32x32 -> ~76x90 px, "START" 64x16 ->
// 176x90 px. As letras (16x16 -> 56x90) têm o miolo de 40 px centrado a 60 px do topo. O botão sai redondo na tela,
// com ~58 px (como o Ⓐ do Enter, um pouco maior que as letras) e centrado na altura delas.
int pad_icon_geom(int btn, int w, int h, ICON_GEOM *g) {
    if (btn >= BTN_A && btn <= BTN_Y && w == 32 && h == 32) {
        g->ay = (90.0f / 32) / (76.0f / 32); g->rad = 12.2f; g->cx = 13.0f; g->cy = 20.8f; return 1;
    }
    if (btn == BTN_BACK && w == 64 && h == 16) {
        g->ay = (90.0f / 16) / (176.0f / 64); g->rad = 10.55f; g->cx = 11.15f; g->cy = 10.67f; return 1;
    }
    return 0;
}
// o "START" do PS1 (0x0b) avança 50; com o ícone redondo do BACK no lugar, avança como os outros ícones
int pad_icon_advance(int btn) { return btn == BTN_BACK ? 17 : 0; }
int pad_menu_back(unsigned now, unsigned prompt_tick) {
    return prompt_tick != 0 && (unsigned)(now - prompt_tick) < 300u;
}
