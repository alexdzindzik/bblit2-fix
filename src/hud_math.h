// Contas puras do HUD (sem Windows nem OpenGL), testadas em tests/test_hud_math.c
#ifndef HUD_MATH_H
#define HUD_MATH_H

enum { HUD_NONE = 0, HUD_SPRITE = 1, HUD_BAR_FILL = 2, HUD_3D = 3 };

// tipo de uma textura do HUD pelo hash de desenho (tex_lhash)
int hud_kind(unsigned long long h);

#endif
