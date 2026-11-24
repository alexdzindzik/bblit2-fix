// Analógico do controle XInput -> analógico do controle do PS1 (contas puras, testadas no Linux)
#include <math.h>
#include "stick_shape.h"

void stick_shape(double x, double y, int dead, int start, int outer, double *ox, double *oy) {
    double m = sqrt(x * x + y * y), dz = dead / 100.0, out = outer / 100.0;
    if (m <= dz || m < 1e-6) { *ox = *oy = 0; return; }
    double t = out > dz ? (m - dz) / (out - dz) : 1.0;
    if (t > 1) t = 1;
    double st = start / 100.0;
    t = st + (1.0 - st) * t;
    double dx = x / m, dy = y / m, k = fabs(dx) > fabs(dy) ? fabs(dx) : fabs(dy);
    *ox = dx / k * t; *oy = dy / k * t;
}

unsigned char stick_ps1(double v) {
    int b = (int)(v * 127.5 + 127.5);
    return (unsigned char)(b < 0 ? 0 : b > 255 ? 255 : b);
}
