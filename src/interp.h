// Interpolação entre dois ticks do jogo (60 fps, etapa 2). Puro: sem OpenGL nem Windows.
// Um ponto do pool do jogo é { X de tela, Y de tela, z de vista }, com tela = vista × K / D(z) e D(z) = z + 1 (z ≥ 0)
// ou −z (z < 0) — a projeção de 0x408d00. Interpolar no espaço de vista: (X·D, Y·D, z); K se cancela.
#ifndef INTERP_H
#define INTERP_H

double ip_div(double z);
// a, b = { X, Y, z } dos ticks A e B; out = ponto em alpha (0 = A, 1 = B)
void ip_vertex(const float a[3], const float b[3], double alpha, float out[3]);
// distância entre os dois pontos no espaço de vista (na escala de K)
double ip_dist(const float a[3], const float b[3]);
// glTexCoord4f(s·q, t·q, r, q) com q = 1/|z|: troca q pelo do z novo, mantendo s e t (q = 0 ou z = 0: copia)
void ip_texcoord(const double tc[4], double z, double out[4]);

// segunda metade da chave de um ponto: ocorrência no quadro (4 bits), número de pontos do bloco/malha (14 bits) e
// número do ponto (14 bits). Uma malha trocada por outra de tamanho diferente (animação) não casa com a anterior.
unsigned ip_key1(unsigned oc, unsigned n, unsigned i);
unsigned ip_key_obj(unsigned k1);
// ponto atrás da câmera (ou colado nela): z > −2, ou os valores que o cenário grava nesse caso (sentinela, 1,0)
int ip_behind(float z, float sentinel);

// salto entre A e B (corte de câmera, teletransporte, par trocado): distância de vista / K maior que o limite.
// Normal: 0,5·|z de B| + jump. Sprite (chave com IP_K0_SPRITE): 0,15·|z de B| + jump/5, mais apertado porque a
// ocorrência de um sprite pode trocar de dono quando um objeto entra ou sai da tela.
// Sprite 2D (|z| < 2 nos dois ticks, z de tela): distância na tela > jump/2.
#define IP_K0_SPRITE 0x80000000u
int ip_jump(const float a[3], const float b[3], double K, double jump, int sprite);
// chave de sprite: k0 = IP_K0_SPRITE | (ocorrência >> 4, 8 bits) << 23 | local da chamada (código do jogo < 0x800000);
// k1 = ip_key1(ocorrência, n, i). Cabem 4096 ocorrências por local (ip_key1 sozinho guarda só 16).
void ip_sprite_key(unsigned site, unsigned oc, unsigned n, unsigned i, unsigned *k0, unsigned *k1);
// grupo que sai inteiro como em B quando um ponto falha (sem par ou salto): (k0, ocorrência & 15) para cenário e
// malhas; para sprites, o local da chamada inteiro, para peças vizinhas do mesmo desenho (as faixas do céu, por
// exemplo) não ficarem metade interpoladas e metade como em B, com um vão entre elas
void ip_bad_group(unsigned k0, unsigned k1, unsigned *g0, unsigned *g1);

// limites de tela { x mín, x máx, y mín, y máx } multiplicados por fx e fy em volta do centro
void ip_widen(const float b[4], float fx, float fy, float out[4]);

// mapa (k0, k1) -> índice >= 0, endereçamento aberto; cap potência de 2; guarda sempre uma vaga (máx. cap − 1)
typedef struct { unsigned *key0, *key1; int *val; int cap, n; } IP_MAP;
void ip_map_init(IP_MAP *m, unsigned *k0, unsigned *k1, int *v, int cap);
void ip_map_clear(IP_MAP *m);
int ip_map_put(IP_MAP *m, unsigned k0, unsigned k1, int val);   // 1 = guardou; 0 = cheio
int ip_map_get(const IP_MAP *m, unsigned k0, unsigned k1);      // −1 = não tem

#endif
