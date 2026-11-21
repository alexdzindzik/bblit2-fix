// Centraliza os menus "nome ... valor" do jogo (ex.: Configuração > Som): os nomes vêm alinhados à direita num x e os
// valores à esquerda noutro, e com os nomes longos do português o bloco sai deslocado. Contas puras, testadas no Linux.
#ifndef MENU_CENTER_H
#define MENU_CENTER_H
enum { MC_LEFT = 0x07, MC_CENTER = 0x08, MC_RIGHT = 0x09 };
typedef struct { int align, x, y, xoff; float w; } MC_RUN;   // xoff: posição (no texto) dos 2 bytes do x
// lê um texto do jogo (ops: byte do op, tamanho de 2 bytes big-endian com o cabeçalho, dados; 0x18 = fim) e devolve
// um trecho por op de texto 0x0a, com o x/alinhamento e o y em vigor e a largura (adv[c] * scale; espaço = 6)
int mc_scan(const unsigned char *s, int max, const unsigned char *adv, float scale, MC_RUN *out, int nmax);
// trechos de um quadro inteiro: marca em use[] os que formam um menu nome/valor (linhas com nome à direita e valor à
// esquerda no mesmo y, ao menos 2) e devolve o quanto somar ao x deles para o bloco ficar centrado (0 = nenhum menu)
int mc_shift(const MC_RUN *r, int n, unsigned char *use);
#endif
