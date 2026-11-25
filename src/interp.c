// Interpolação entre dois ticks (ver interp.h)
#include <math.h>
#include "interp.h"

double ip_div(double z) { return z >= 0 ? z + 1 : -z; }

void ip_vertex(const float a[3], const float b[3], double alpha, float out[3]) {
    double da = ip_div(a[2]), db = ip_div(b[2]);
    double x = a[0] * da + (b[0] * db - a[0] * da) * alpha;
    double y = a[1] * da + (b[1] * db - a[1] * da) * alpha;
    double z = a[2] + ((double)b[2] - a[2]) * alpha;
    double d = ip_div(z);
    out[0] = (float)(x / d); out[1] = (float)(y / d); out[2] = (float)z;
}

double ip_dist(const float a[3], const float b[3]) {
    double da = ip_div(a[2]), db = ip_div(b[2]);
    double dx = b[0] * db - a[0] * da, dy = b[1] * db - a[1] * da, dz = (double)b[2] - a[2];
    return sqrt(dx * dx + dy * dy + dz * dz);
}

int ip_jump(const float a[3], const float b[3], double K, double jump, int sprite) {
    if (sprite && fabs(a[2]) < 2 && fabs(b[2]) < 2)    // sprite 2D (z de tela): distância em pixels
        return hypot((double)b[0] - a[0], (double)b[1] - a[1]) > jump / 2;
    double z = fabs(b[2]);
    double lim = sprite ? 0.15 * z + jump / 5 : 0.5 * z + jump;
    return ip_dist(a, b) / K > lim;
}

void ip_sprite_key(unsigned site, unsigned oc, unsigned n, unsigned i, unsigned *k0, unsigned *k1) {
    *k0 = IP_K0_SPRITE | ((oc >> 4) & 0xFF) << 23 | (site & 0x7FFFFF);
    *k1 = ip_key1(oc, n, i);
}

void ip_bad_group(unsigned k0, unsigned k1, unsigned *g0, unsigned *g1) {
    if (k0 & IP_K0_SPRITE) { *g0 = IP_K0_SPRITE | (k0 & 0x7FFFFF); *g1 = 0x10; }
    else { *g0 = k0; *g1 = ip_key_obj(k1); }
}

void ip_texcoord(const double tc[4], double z, double out[4]) {
    out[0] = tc[0]; out[1] = tc[1]; out[2] = tc[2]; out[3] = tc[3];
    if (tc[3] == 0 || z == 0) return;
    double q = 1.0 / fabs(z);
    out[0] = tc[0] / tc[3] * q; out[1] = tc[1] / tc[3] * q; out[3] = q;
}

unsigned ip_key1(unsigned oc, unsigned n, unsigned i) { return (oc & 0xF) << 28 | (n & 0x3FFF) << 14 | (i & 0x3FFF); }
unsigned ip_key_obj(unsigned k1) { return k1 >> 28; }
int ip_behind(float z, float sentinel) { return z > -2 || z == sentinel || z == 1.0f; }

void ip_widen(const float b[4], float fx, float fy, float out[4]) {
    double cx = (b[0] + (double)b[1]) / 2, hx = (b[1] - (double)b[0]) / 2 * fx;
    double cy = (b[2] + (double)b[3]) / 2, hy = (b[3] - (double)b[2]) / 2 * fy;
    out[0] = (float)(cx - hx); out[1] = (float)(cx + hx); out[2] = (float)(cy - hy); out[3] = (float)(cy + hy);
}

static unsigned ip_hash(unsigned k0, unsigned k1) { return k0 * 2654435761u ^ (k1 + 0x9e3779b9u) * 40503u; }

void ip_map_init(IP_MAP *m, unsigned *k0, unsigned *k1, int *v, int cap) {
    m->key0 = k0; m->key1 = k1; m->val = v; m->cap = cap;
    ip_map_clear(m);
}

void ip_map_clear(IP_MAP *m) {
    for (int i = 0; i < m->cap; i++) m->val[i] = -1;
    m->n = 0;
}

int ip_map_put(IP_MAP *m, unsigned k0, unsigned k1, int val) {
    unsigned mask = (unsigned)m->cap - 1, i = ip_hash(k0, k1) & mask;
    while (m->val[i] >= 0) {
        if (m->key0[i] == k0 && m->key1[i] == k1) { m->val[i] = val; return 1; }
        i = (i + 1) & mask;
    }
    if (m->n >= m->cap - 1) return 0;
    m->key0[i] = k0; m->key1[i] = k1; m->val[i] = val; m->n++;
    return 1;
}

int ip_map_get(const IP_MAP *m, unsigned k0, unsigned k1) {
    unsigned mask = (unsigned)m->cap - 1, i = ip_hash(k0, k1) & mask;
    while (m->val[i] >= 0) {
        if (m->key0[i] == k0 && m->key1[i] == k1) return m->val[i];
        i = (i + 1) & mask;
    }
    return -1;
}
