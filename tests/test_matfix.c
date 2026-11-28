// testes de mf_unwrap (matfix.c)
#include <stdio.h>
#include <math.h>
#include "../src/matfix.h"

static int fails, checks;
static void check(int ok, const char *what) { checks++; if (!ok) { fails++; printf("FALHOU: %s\n", what); } }

// O que o jogo grava: soma de 32 bits, sar 12, byte baixo em short
static void game_mul(const short C[9], const short W[9], short O[9]) {
    for (int i = 0; i < 3; i++)
        for (int j = 0; j < 3; j++) {
            int s = 0;
            for (int k = 0; k < 3; k++) s += C[i * 3 + k] * W[k * 3 + j];
            O[i * 3 + j] = (short)(s >> 12);
        }
}

int main(void) {
    float f[9];
    // palmeira do hub árabe em 4K: câmera com escala de tela (linha y ≈ -5,64) × nó com escala 2 (8192)
    short cam[9] = { 22000, 0, -3000, 1100, -23085, 900, -1400, -200, 3800 };
    short w2[9] = { 8192, 0, 0, 0, 8190, 124, 0, -124, 8190 };
    short o[9]; game_mul(cam, w2, o);
    check(o[4] > 0, "o jogo grava a linha y com o sinal trocado (volta do short)");
    check(mf_unwrap(cam, w2, o, f) == 1, "palmeira: estouro detectado");
    check(fabsf(f[4] - (-23085.0f * 8190 + 900.0f * -124) / 4096 / 4096) < 0.001f, "palmeira: termo y certo (-11,28)");
    check(fabsf(f[0] - 22000.0f * 8192 / 4096 / 4096) < 0.001f, "palmeira: termo x certo (10,74)");
    int same = 1;   // os termos sem estouro ficam iguais ao que a conversão do jogo daria (short / 4096)
    for (int k = 0; k < 9; k++) if (fabsf(f[k]) < 8.0f && f[k] * 4096.0f != (float)o[k]) same = 0;
    check(same, "palmeira: termos sem estouro iguais aos do jogo");

    // personagem (escala 1): nada estoura, nada muda
    short w1[9] = { 4096, 0, 0, 0, 4096, 0, 0, 0, 4096 };
    game_mul(cam, w1, o);
    check(mf_unwrap(cam, w1, o, f) == 0, "escala 1: sem estouro, devolve 0");

    // O diferente do produto (outro caminho mexeu na matriz): não mexe
    game_mul(cam, w2, o); o[8] += 1;
    check(mf_unwrap(cam, w2, o, f) == 0, "O inesperado: devolve 0");

    // soma que estouraria 32 bits no jogo: não mexe
    short big[9] = { 32767, 32767, 32767, 0, 4096, 0, 0, 0, 4096 };
    short wb[9] = { 32767, 0, 0, 32767, 0, 0, 32767, 0, 0 };
    game_mul(big, wb, o);
    check(mf_unwrap(big, wb, o, f) == 0, "estouro de 32 bits: devolve 0");

    printf("%d/%d verificações ok\n", checks - fails, checks);
    return fails != 0;
}
