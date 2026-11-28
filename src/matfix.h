// Rotação câmera × nó dos modelos sem o estouro de 16 bits (0x4048b0, MulMatrix0, guarda cada termo em short 4.12).
#ifndef MATFIX_H
#define MATFIX_H

// C: rotação da câmera; W: rotação do nó no mundo; O: o que o jogo gravou, (C·W) >> 12 em short. Tudo 3×3 em 4.12
// (4096 = 1,0). Refaz cada termo com 64 bits. Se algum passou do short (|valor| ≥ 8,0) e todos os shorts do jogo batem
// com o resultado truncado (a única diferença é a volta), escreve em f a rotação certa (termo / 4096) e devolve 1.
// Sem estouro, com um termo que o jogo somaria com estouro em 32 bits, ou com O diferente do esperado: devolve 0.
int mf_unwrap(const short C[9], const short W[9], const short O[9], float f[9]);

#endif
