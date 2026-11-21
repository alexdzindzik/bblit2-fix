// Cursor do mouse escondido enquanto o controle navega pelos menus 2D do início (contas puras, testadas no Linux)
#ifndef PAD_CURSOR_H
#define PAD_CURSOR_H
enum { PADCUR_KEEP = 0, PADCUR_HIDE = 1, PADCUR_SHOW = 2 };
typedef struct { int hidden; long x, y; } PADCUR;   // x, y: onde o controle deixou o cursor (tela)
int padcur_nav(PADCUR *s, long x, long y);          // o controle levou o cursor até (x, y)
int padcur_mouse(PADCUR *s, long x, long y);        // evento do mouse com o cursor em (x, y)
int padcur_leave(PADCUR *s);                        // saiu dos menus 2D (fase, vídeo, foco perdido)
#endif
