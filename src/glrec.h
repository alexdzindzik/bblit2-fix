// Instantâneo dos comandos OpenGL de um tick do jogo (60 fps, etapa 1). Puro: sem OpenGL nem Windows.
// Cada comando guarda o código (op), até 3 inteiros (u) e até 6 números (d). Uso por comando:
//   VERTEX d = x, y, z e u[0] = ponteiro do jogo; COLOR d = r, g, b, a; TEXCOORD d = s, t, r, q;
//   BIND u = alvo, nome; ENABLE/DISABLE u[0]; BLENDFUNC u[0..1]; ALPHAFUNC u[0], d[0];
//   DEPTHFUNC/SHADEMODEL/CLEAR/MATRIXMODE u[0]; DEPTHRANGE d[0..1]; POLYGONMODE/HINT u[0..1];
//   CLEARCOLOR d[0..3]; VIEWPORT/SCISSOR d[0..3]; ORTHO d[0..5]; TEXPARAM u = alvo, parâmetro, d[0] = valor.
#ifndef GLREC_H
#define GLREC_H

enum { GR_BEGIN = 1, GR_END, GR_VERTEX, GR_COLOR, GR_TEXCOORD, GR_BIND, GR_ENABLE, GR_DISABLE, GR_BLENDFUNC,
       GR_ALPHAFUNC, GR_DEPTHFUNC, GR_DEPTHRANGE, GR_SHADEMODEL, GR_POLYGONMODE, GR_HINT, GR_CLEARCOLOR, GR_CLEAR,
       GR_VIEWPORT, GR_SCISSOR, GR_MATRIXMODE, GR_LOADIDENTITY, GR_ORTHO, GR_TEXPARAM, GR_N };
#define GR_RAW 0x100   // bit no op: aplicar direto no OpenGL, sem passar pelos ganchos do mod (estado inicial)

typedef struct { unsigned op, u[3]; double d[6]; } GR_CMD;
typedef struct { GR_CMD *cmd; int n, cap, overflow; } GR_SNAP;

void gr_init(GR_SNAP *s, GR_CMD *buf, int cap);
void gr_reset(GR_SNAP *s);
// 1 = gravou; 0 = cheio (marca overflow). u e d que não forem passados ficam 0.
int gr_put(GR_SNAP *s, unsigned op, const unsigned *u, int nu, const double *d, int nd);
// repete os comandos na ordem; devolve quantos. Com overflow (instantâneo incompleto): 0, sem chamar fn.
int gr_replay(const GR_SNAP *s, void (*fn)(const GR_CMD *c, void *ctx), void *ctx);

#endif
