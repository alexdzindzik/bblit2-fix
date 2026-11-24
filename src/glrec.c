// Instantâneo dos comandos OpenGL de um tick (ver glrec.h)
#include <string.h>
#include "glrec.h"

void gr_init(GR_SNAP *s, GR_CMD *buf, int cap) {
    s->cmd = buf; s->cap = buf ? cap : 0; s->n = 0; s->overflow = 0;
}

void gr_reset(GR_SNAP *s) { s->n = 0; s->overflow = 0; }

int gr_put(GR_SNAP *s, unsigned op, const unsigned *u, int nu, const double *d, int nd) {
    if (s->n >= s->cap) { s->overflow = 1; return 0; }
    GR_CMD *c = &s->cmd[s->n++];
    memset(c, 0, sizeof *c);
    c->op = op;
    for (int i = 0; i < nu && i < 3; i++) c->u[i] = u[i];
    for (int i = 0; i < nd && i < 6; i++) c->d[i] = d[i];
    return 1;
}

int gr_replay(const GR_SNAP *s, void (*fn)(const GR_CMD *c, void *ctx), void *ctx) {
    if (s->overflow) return 0;
    for (int i = 0; i < s->n; i++) fn(&s->cmd[i], ctx);
    return s->n;
}
