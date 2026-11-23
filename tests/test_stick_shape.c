// Testes do analógico XInput -> PS1 (rodam no Linux): bash build.sh
#include <math.h>
#include <stdio.h>
#include "../src/stick_shape.h"

static int g_fail, g_run;
#define CHECK(c) do { g_run++; if (!(c)) { g_fail++; printf("FALHOU %s:%d: %s\n", __FILE__, __LINE__, #c); } } while (0)
#define NEAR(a, b) (fabs((a) - (b)) < 0.01)

static void test_dead_zone(void) {
    double x, y;
    stick_shape(0, 0, 10, 20, 95, &x, &y);        CHECK(x == 0 && y == 0);
    stick_shape(0.09, 0, 10, 20, 95, &x, &y);     CHECK(x == 0 && y == 0);   // dentro da zona morta
    stick_shape(-0.05, 0.05, 10, 20, 95, &x, &y); CHECK(x == 0 && y == 0);   // zona morta redonda
}
static void test_first_tilt_walks(void) {
    double x, y;
    stick_shape(0.11, 0, 10, 20, 95, &x, &y);     CHECK(x > 0.19 && x < 0.22 && y == 0);   // logo depois: ~start
}
static void test_outer_is_full(void) {
    double x, y;
    stick_shape(0.95, 0, 10, 20, 95, &x, &y);     CHECK(NEAR(x, 1) && y == 0);
    stick_shape(0, -0.85, 10, 20, 80, &x, &y);    CHECK(x == 0 && NEAR(y, -1));   // controle gasto: 85% para baixo
    stick_shape(0, -0.85, 10, 20, 95, &x, &y);    CHECK(y > -0.95);               // com 95% ainda não é o máximo
}
static void test_diagonal_is_full_on_both_axes(void) {
    double x, y, d = sqrt(0.5);
    stick_shape(-d, d, 10, 20, 95, &x, &y);       CHECK(NEAR(x, -1) && NEAR(y, 1));   // cima-esquerda, inclinação toda
    stick_shape(d * 0.5, d * 0.5, 10, 0, 100, &x, &y); CHECK(NEAR(x, y) && x > 0 && x < 1);   // meia diagonal: meio
}
static void test_symmetric(void) {
    double ax, ay, bx, by;
    stick_shape(0.6, 0, 10, 20, 95, &ax, &ay);
    stick_shape(-0.6, 0, 10, 20, 95, &bx, &by);  CHECK(NEAR(ax, -bx));   // direita e esquerda iguais
}
static void test_ps1_bytes(void) {
    CHECK(stick_ps1(-1) == 0); CHECK(stick_ps1(0) == 127); CHECK(stick_ps1(1) == 255);
    CHECK(stick_ps1(-2) == 0); CHECK(stick_ps1(2) == 255);
}

int main(void) {
    test_dead_zone();
    test_first_tilt_walks();
    test_outer_is_full();
    test_diagonal_is_full_on_both_axes();
    test_symmetric();
    test_ps1_bytes();
    printf("%d/%d verificações ok\n", g_run - g_fail, g_run);
    return g_fail ? 1 : 0;
}
