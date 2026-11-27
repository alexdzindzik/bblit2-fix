// Matiz, saturação e brilho do menu Vídeo do jogo, no modelo HSI que o jogo usa nas paletas
// (0x408270 RGB -> HSI, 0x407d30 ajuste, 0x407fe0 HSI -> RGB). Cores de 0 a 1, matiz em graus.
#ifndef COLORADJ_H
#define COLORADJ_H

// I = (r + g + b) / 3; S = 1 - 3·mín / (r + g + b); H = acos(...) em graus, 360 - H se b > g. Cinza: H = S = 0.
void ca_rgb2hsi(double r, double g, double b, double *h, double *s, double *i);
// inversa, por setores de 120°
void ca_hsi2rgb(double h, double s, double i, double *r, double *g, double *b);

// O jogo grava cada canal como o byte baixo de cor·255 truncado (0x407eb4..0x407edd, sem limite): cor clara e
// saturada com o Brilho alto passa de 255 e "dá a volta" (382 vira 126), e a pele da Vovó e do Piu-Piu mancha.
// Refaz a conta do canal como o jogo (H + matiz, volta em 360; S · saturação e I · brilho, no máximo 1) e, onde
// ela passou de 255 ou ficou abaixo de 0, troca o byte do jogo pelo limite.
// orig: cor da paleta antes do ajuste; game: o que o jogo gravou (corrigido no lugar). Devolve os canais trocados.
int ca_fix_wrap(const unsigned char orig[3], unsigned char game[3], double hue, double sat, double val);

// O mesmo ajuste numa cor por vértice (partes sem textura): H + matiz, S · saturação e I · brilho (no máximo 1),
// canais limitados a 0..1. Matiz 0, saturação 1 e brilho 1 devolvem a cor exatamente igual.
void ca_adjust(const float in[3], float out[3], double hue, double sat, double val);

#endif
