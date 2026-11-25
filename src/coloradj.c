// Ajuste de cor do menu Vídeo no modelo HSI do jogo (ver coloradj.h)
#include <math.h>
#include "coloradj.h"

static double cosd(double d) { return cos(d * 3.14159265358979323846 / 180); }

void ca_rgb2hsi(double r, double g, double b, double *h, double *s, double *i) {
    double sum = r + g + b, mn = fmin(r, fmin(g, b));
    *i = sum / 3;
    if (r == g && g == b) { *h = 0; *s = 0; return; }
    *s = 1 - 3 * mn / sum;
    double num = 0.5 * ((r - g) + (r - b)), den = sqrt((r - g) * (r - g) + (r - b) * (g - b));
    double c = den > 0 ? num / den : 1;
    if (c > 1) c = 1;
    if (c < -1) c = -1;
    *h = acos(c) * 57.29577951;
    if (b > g) *h = 360 - *h;
}

void ca_hsi2rgb(double h, double s, double i, double *r, double *g, double *b) {
    h = fmod(h, 360);
    if (h < 0) h += 360;
    double x = i * (1 - s);
    if (h < 120) { *b = x; *r = i * (1 + s * cosd(h) / cosd(60 - h)); *g = 3 * i - (*r + *b); }
    else if (h < 240) { h -= 120; *r = x; *g = i * (1 + s * cosd(h) / cosd(60 - h)); *b = 3 * i - (*r + *g); }
    else { h -= 240; *g = x; *b = i * (1 + s * cosd(h) / cosd(60 - h)); *r = 3 * i - (*g + *b); }
}

int ca_fix_wrap(const unsigned char orig[3], unsigned char game[3], double hue, double sat, double val) {
    double h, s, i, c[3];
    ca_rgb2hsi(orig[0] / 255.0, orig[1] / 255.0, orig[2] / 255.0, &h, &s, &i);
    h += hue;
    if (h > 360) h -= 360;
    s *= sat;
    if (s > 1) s = 1;
    i *= val;
    if (i > 1) i = 1;
    ca_hsi2rgb(h, s, i, &c[0], &c[1], &c[2]);
    int n = 0;
    for (int k = 0; k < 3; k++) {
        long v = (long)(c[k] * 255);                    // o jogo trunca (fistp com corte)
        if (v > 255 && game[k] != 255) { game[k] = 255; n++; }
        else if (v < 0 && game[k] != 0) { game[k] = 0; n++; }
    }
    return n;
}
