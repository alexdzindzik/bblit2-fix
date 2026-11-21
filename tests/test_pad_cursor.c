// Testes do cursor escondido na navegação pelo controle nos menus 2D (rodam no Linux): bash build.sh
#include <stdio.h>
#include "../src/pad_cursor.h"

static int g_fail, g_run;
#define CHECK(c) do { g_run++; if (!(c)) { g_fail++; printf("FALHOU %s:%d: %s\n", __FILE__, __LINE__, #c); } } while (0)

static void test_nav_hides_once(void) {
    PADCUR s = { 0 };
    CHECK(padcur_nav(&s, 100, 200) == PADCUR_HIDE);      // o controle moveu o cursor: some
    CHECK(padcur_nav(&s, 300, 200) == PADCUR_KEEP);      // já escondido: nada a fazer
    CHECK(s.hidden == 1);
}
static void test_own_mousemove_keeps_hidden(void) {
    PADCUR s = { 0 };
    padcur_nav(&s, 100, 200);
    CHECK(padcur_mouse(&s, 100, 200) == PADCUR_KEEP);    // o WM_MOUSEMOVE que o próprio mod gerou
    CHECK(padcur_mouse(&s, 102, 198) == PADCUR_KEEP);    // tremida de 2 pixels
}
static void test_real_mouse_shows(void) {
    PADCUR s = { 0 };
    padcur_nav(&s, 100, 200);
    CHECK(padcur_mouse(&s, 140, 200) == PADCUR_SHOW);    // o mouse de verdade andou: volta
    CHECK(s.hidden == 0);
    CHECK(padcur_mouse(&s, 300, 300) == PADCUR_KEEP);    // já visível
}
static void test_leave_menu_shows(void) {
    PADCUR s = { 0 };
    CHECK(padcur_leave(&s) == PADCUR_KEEP);              // visível: nada a fazer
    padcur_nav(&s, 10, 10);
    CHECK(padcur_leave(&s) == PADCUR_SHOW);              // saiu dos menus com o cursor escondido: devolve
    CHECK(s.hidden == 0);
}

int main(void) {
    test_nav_hides_once();
    test_own_mousemove_keeps_hidden();
    test_real_mouse_shows();
    test_leave_menu_shows();
    printf("%d/%d verificações ok\n", g_run - g_fail, g_run);
    return g_fail ? 1 : 0;
}
