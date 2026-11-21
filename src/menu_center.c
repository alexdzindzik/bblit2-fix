#include "menu_center.h"
int mc_scan(const unsigned char *s, int max, const unsigned char *adv, float scale, MC_RUN *out, int nmax) {
    int n = 0, align = 0, x = 0, y = 0, xoff = -1;
    for (int o = 0; o + 3 <= max && n < nmax;) {
        int code = s[o], len = s[o + 1] << 8 | s[o + 2];
        if (code == 0x18) break;                          // fim do texto
        if (len < 3 || o + len > max) break;              // tamanho inválido: para
        const unsigned char *d = s + o + 3; int dn = len - 3;
        if ((code == MC_LEFT || code == MC_CENTER || code == MC_RIGHT) && dn >= 2) {
            align = code; x = (short)(d[0] << 8 | d[1]); xoff = o + 3;
        } else if (code == 0x04 && dn >= 2) {
            y = (short)(d[0] << 8 | d[1]);
        } else if (code == 0x0a && dn >= 1 && xoff >= 0) {
            float w = 0;
            for (int i = 1; i < dn; i++) w += d[i] == ' ' ? 6.0f : adv[d[i]] * scale;
            out[n].align = align; out[n].x = x; out[n].y = y; out[n].xoff = xoff; out[n].w = w; n++;
        }
        o += len;
    }
    return n;
}
int mc_shift(const MC_RUN *r, int n, unsigned char *use) {
    int rows = 0;
    for (int i = 0; i < n; i++) {                         // linha do menu: um nome à direita e um valor à esquerda no mesmo y
        use[i] = 0;
        if (r[i].align != MC_RIGHT && r[i].align != MC_LEFT) continue;
        int want = r[i].align == MC_RIGHT ? MC_LEFT : MC_RIGHT;
        for (int j = 0; j < n; j++) if (r[j].align == want && r[j].y == r[i].y) { use[i] = 1; break; }
        if (use[i] && r[i].align == MC_RIGHT) rows++;
    }
    if (rows < 2) { for (int i = 0; i < n; i++) use[i] = 0; return 0; }
    float left = 1e9f, right = -1e9f;
    for (int i = 0; i < n; i++) {
        if (!use[i]) continue;
        float a = r[i].align == MC_RIGHT ? r[i].x - r[i].w : (float)r[i].x, b = r[i].align == MC_RIGHT ? (float)r[i].x : r[i].x + r[i].w;
        if (a < left) left = a;
        if (b > right) right = b;
    }
    float c = (left + right) / 2;
    return (int)(c < 0 ? -c + 0.5f : -(c + 0.5f));
}
