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

// ponto do cenário como 0x408990 grava no pool: v = 3 shorts do bloco, m = matriz da câmera (12 floats, 0x63c020):
// vista = linhas m[0..2], m[3..5], m[6..8] · v + m[9..11]. Vista z < 0: { X·K/−z, Y·K/−z, z }; z = 0: { X·K, Y·K, 1 };
// z > 0: { 2X, 2Y, sentinela }. x ou y de v fora de ±32000: 0 (o jogo marca o ponto invisível).
int ip_room_point(const float m[12], float K, float sentinel, const short v[3], float out[3]);

// mapa (k0, k1) -> índice >= 0, endereçamento aberto; cap potência de 2; guarda sempre uma vaga (máx. cap − 1)
typedef struct { unsigned *key0, *key1; int *val; int cap, n; } IP_MAP;
void ip_map_init(IP_MAP *m, unsigned *k0, unsigned *k1, int *v, int cap);
void ip_map_clear(IP_MAP *m);
int ip_map_put(IP_MAP *m, unsigned k0, unsigned k1, int val);   // 1 = guardou; 0 = cheio
int ip_map_get(const IP_MAP *m, unsigned k0, unsigned k1);      // −1 = não tem

#endif
