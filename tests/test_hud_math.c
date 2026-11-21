// Testes das contas puras do HUD (rodam no Linux, antes da compilação da DLL): bash build.sh
#include <stdio.h>
#include <stdlib.h>
#include "../src/hud_math.h"

static int g_fail, g_run;
#define CHECK(c) do { g_run++; if (!(c)) { g_fail++; printf("FALHOU %s:%d: %s\n", __FILE__, __LINE__, #c); } } while (0)

static void test_hud_kind(void) {
    CHECK(hud_kind(0x3ff28889dc2f9625ULL) == HUD_SPRITE);    // silhueta do Taz
    CHECK(hud_kind(0x6153c0570df99ae9ULL) == HUD_SPRITE);    // silhueta do Pernalonga
    CHECK(hud_kind(0x16463c6abc682e5cULL) == HUD_BAR_FILL);  // preenchimento da barra
    CHECK(hud_kind(0x6da516a4789678d7ULL) == HUD_BAR_FILL);
    CHECK(hud_kind(0xcdfbd6edf13ed09eULL) == HUD_3D);        // cenoura (modelo 3D)
    CHECK(hud_kind(0x76e79fa1ab5f0475ULL) == HUD_3D);        // claquete (modelo 3D)
    CHECK(hud_kind(0x1234ULL) == HUD_NONE);
}

int main(void) {
    test_hud_kind();
    printf("%d/%d verificações ok\n", g_run - g_fail, g_run);
    return g_fail ? 1 : 0;
}
