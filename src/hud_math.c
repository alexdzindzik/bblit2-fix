// Contas puras do HUD (sem Windows nem OpenGL), testadas em tests/test_hud_math.c
#include "hud_math.h"
#include <stddef.h>

// texturas do HUD da versão PT-BR (hash de desenho, igual em qualquer ajuste de "Brilho")
static const struct { unsigned long long h; int kind; } k_hud[] = {
    { 0x3ff28889dc2f9625ULL, HUD_SPRITE },   // silhueta do Taz + moldura da barra (128x64)
    { 0x6153c0570df99ae9ULL, HUD_SPRITE },   // silhueta do Pernalonga + moldura da barra (256x64)
    { 0x16463c6abc682e5cULL, HUD_BAR_FILL }, // preenchimento das barras de vida (128x8)
    { 0x6da516a4789678d7ULL, HUD_BAR_FILL },
    { 0x8a24a8a3dc058391ULL, HUD_SPRITE }, { 0xdb0a957bf8a45c7eULL, HUD_SPRITE },   // números amarelos 0..9
    { 0x5e0ceaf077778bb8ULL, HUD_SPRITE }, { 0x1cdb8f624aa4c960ULL, HUD_SPRITE },
    { 0x92580ff79274a8f4ULL, HUD_SPRITE }, { 0x593232224dc3f8c3ULL, HUD_SPRITE },
    { 0x08757f739ab6f819ULL, HUD_SPRITE }, { 0x4f1dd2973af8b13eULL, HUD_SPRITE },
    { 0xe72f6db3dc68fc82ULL, HUD_SPRITE }, { 0xf07de88f84ea054aULL, HUD_SPRITE },
    { 0x88d23ca06bc73a6cULL, HUD_SPRITE },   // %
    { 0x81186b738d5b2629ULL, HUD_SPRITE },   // espelho
    { 0xc71b14dfca3365ceULL, HUD_SPRITE },   // espelho riscado
    { 0x7052f27c1a3671b8ULL, HUD_SPRITE },   // X
    { 0xbef4c66e2a96838bULL, HUD_SPRITE },   // mira
    { 0x4c6bdfe92ed18a2dULL, HUD_SPRITE },   // relógio
    { 0x2263ff8efc8e2c56ULL, HUD_SPRITE }, { 0x48b3a3ee49d533c1ULL, HUD_SPRITE },   // medalhas
    { 0x3abd3f8592615d20ULL, HUD_SPRITE }, { 0xd60f246eed512e05ULL, HUD_SPRITE },
    { 0x59f8e33e8c711edeULL, HUD_SPRITE },   // emblema
    { 0x9f99c3526e3e26aeULL, HUD_SPRITE },   // barra laranja
    { 0x1773bbcebaff6d1fULL, HUD_SPRITE }, { 0x814438235e46e7fcULL, HUD_SPRITE },   // setas
    { 0x76e79fa1ab5f0475ULL, HUD_3D },       // claquete (textura de modelo 3D)
    { 0xcdfbd6edf13ed09eULL, HUD_3D },       // cenoura (textura de modelo 3D)
};

int hud_kind(unsigned long long h) {
    for (size_t i = 0; i < sizeof k_hud / sizeof *k_hud; i++)
        if (k_hud[i].h == h) return k_hud[i].kind;
    return HUD_NONE;
}
