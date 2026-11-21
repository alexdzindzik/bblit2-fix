#include "pad_icons.h"
// ✕ □ △ ○ do PS1 ficam em 0xac-0xaf na tabela de glifos do jogo (0x4cf270: glifos 103-106); R1 0x10, L1 0x12,
// Esc 0x13 e "START" 0x0b. LT e RT não têm símbolo na fonte.
static const unsigned char k_char[BTN_N] = { 0xac, 0xaf, 0xad, 0xae, 0x12, 0x10, 0, 0, 0x13, 0x0b };
unsigned char pad_icon_char(int btn) { return btn >= 0 && btn < BTN_N ? k_char[btn] : 0; }
