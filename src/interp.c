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

void ip_texcoord(const double tc[4], double z, double out[4]) {
    out[0] = tc[0]; out[1] = tc[1]; out[2] = tc[2]; out[3] = tc[3];
    if (tc[3] == 0 || z == 0) return;
    double q = 1.0 / fabs(z);
    out[0] = tc[0] / tc[3] * q; out[1] = tc[1] / tc[3] * q; out[3] = q;
}

unsigned ip_key1(unsigned oc, unsigned n, unsigned i) { return (oc & 0xF) << 28 | (n & 0x3FFF) << 14 | (i & 0x3FFF); }
unsigned ip_key_obj(unsigned k1) { return k1 >> 28; }
int ip_behind(float z, float sentinel) { return z > -2 || z == sentinel || z == 1.0f; }

int ip_room_point(const float m[12], float K, float sentinel, const short v[3], float out[3]) {
    double x = v[0], y = v[1], z = v[2];
    if (x < -32000 || x > 32000 || y < -32000 || y > 32000) return 0;
    double vz = m[6] * x + m[7] * y + m[8] * z + m[11], f;
    if (vz < 0) { f = -K / vz; out[2] = (float)vz; }
    else if (vz == 0) { f = K; out[2] = 1.0f; }
    else { f = 2; out[2] = sentinel; }
    out[0] = (float)((m[0] * x + m[1] * y + m[2] * z + m[9]) * f);
    out[1] = (float)((m[3] * x + m[4] * y + m[5] * z + m[10]) * f);
    return 1;
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
