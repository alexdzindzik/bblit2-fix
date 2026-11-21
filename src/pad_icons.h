// Botões do Xbox nas dicas: caractere da fonte do jogo que mostra cada ícone redesenhado
#ifndef PAD_ICONS_H
#define PAD_ICONS_H
enum { BTN_A, BTN_B, BTN_X, BTN_Y, BTN_LB, BTN_RB, BTN_LT, BTN_RT, BTN_START, BTN_BACK, BTN_N };
unsigned char pad_icon_char(int btn);   // 0 = sem ícone na fonte (o texto mostra o nome do botão)
#endif
