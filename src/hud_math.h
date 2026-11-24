// Contas puras do HUD (sem Windows nem OpenGL), testadas em tests/test_hud_math.c
#ifndef HUD_MATH_H
#define HUD_MATH_H

enum { HUD_NONE = 0, HUD_SPRITE = 1, HUD_BAR_FILL = 2, HUD_3D = 3 };

// tipo de uma textura do HUD pelo hash de desenho (tex_lhash)
int hud_kind(unsigned long long h);

#include <stddef.h>
#include <stdint.h>

// BMP 32 bits (BGRA, de baixo para cima). Pixels em RGBA como o OpenGL recebe (byte 0 = R), linha 0 = a de
// baixo da textura = primeira linha do arquivo. parse: out NULL só lê o tamanho; devolve 1 se válido.
int bmp32_parse(const unsigned char *buf, size_t n, int *w, int *h, uint32_t *out);
size_t bmp32_build(const uint32_t *rgba, int w, int h, unsigned char *out);   // bytes; out NULL = só o tamanho
unsigned long long rgba_hash(const uint32_t *p, size_t n);                    // FNV-1a 64 do RGBA exato

// fonte do pacote do HUD (bin\hd\hud\fonte\<hash>.bmp + .txt com o hash exato das cores): o que fazer no envio
enum { FONTE_KEEP = 0, FONTE_WRITE = 1, FONTE_REWRITE = 2 };    // REWRITE = cores mudaram (Brilho): gerar o pacote de novo
int hud_fonte_action(int bmp_exists, int txt_ok, unsigned long long txt_hash, unsigned long long cur_hash);

// sombra: alfa (w x h) desfocado (gaussiano separável, sigma por eixo, raio ceil(3*sigma)) numa saída com margem
// (w+2*padx) x (h+2*pady). Valor > 0 nunca arredonda para 0 (a borda da sombra não some).
void hud_shadow_mask(const unsigned char *alpha, int w, int h, int padx, int pady, float sx, float sy, unsigned char *out);

#endif
