// Botões do Xbox nas dicas: caractere da fonte do jogo que mostra cada ícone redesenhado
#ifndef PAD_ICONS_H
#define PAD_ICONS_H
enum { BTN_A, BTN_B, BTN_X, BTN_Y, BTN_LB, BTN_RB, BTN_LT, BTN_RT, BTN_START, BTN_BACK, BTN_N };
unsigned char pad_icon_char(int btn);   // 0 = sem ícone na fonte (o texto mostra o nome do botão)
// botão redondo desenhado na textura do glifo (texels da textura original, de cima para baixo): centro (cx colunas,
// cy linhas), raio em colunas; ay = quantas colunas cabem na altura de uma linha, na tela
typedef struct { float ay, cx, cy, rad; } ICON_GEOM;
int pad_icon_geom(int btn, int w, int h, ICON_GEOM *g);   // 1 = botão redondo nesse glifo
int pad_icon_advance(int btn);                            // novo avanço do caractere na fonte (0 = mantém)
// B também volta ao menu anterior (F4) enquanto a dica do F4 está na tela (o jogo a escreve a cada quadro);
// prompt_tick = GetTickCount da última vez que a dica pediu o nome do F4 (0 = nunca)
int pad_menu_back(unsigned now, unsigned prompt_tick);   // 32 bits, como o GetTickCount
#endif
