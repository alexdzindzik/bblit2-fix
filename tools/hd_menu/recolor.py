# Recolore a tela inicial (anéis azul-claro) com as cores dos anéis escuros da tela de configuração.
import numpy as np
from scipy import ndimage, optimize
BIN = 0.5
LUM = np.array([0.299, 0.587, 0.114])

def radius(h, w, cx, cy, sx, ox=0, oy=0):
    y, x = np.mgrid[0:h, 0:w].astype(np.float64) + 0.5
    return np.sqrt(((x + ox - cx) * sx) ** 2 + (y + oy - cy) ** 2)

def binned_median(vals, r, mask, nb):
    b = np.clip((r[mask] / BIN).astype(int), 0, nb - 1); v = vals[mask]
    o = np.argsort(b, kind='stable'); b = b[o]; v = v[o]
    edges = np.searchsorted(b, np.arange(nb + 1)); out = np.full((nb,) + v.shape[1:], np.nan); cnt = np.diff(edges)
    for i in range(nb):
        if cnt[i] >= 3: out[i] = np.median(v[edges[i]:edges[i + 1]], axis=0)
    return out, cnt

def fit_rings(img, mask):
    """centro e achatamento: minimiza o espalhamento da luminância dentro de cada faixa de raio"""
    lum = img @ LUM; h, w = lum.shape
    yy, xx = np.nonzero(mask); sel = np.random.default_rng(1).choice(len(yy), min(12000, len(yy)), replace=False)
    ys, xs, ls = yy[sel] + 0.5, xx[sel] + 0.5, lum[yy[sel], xx[sel]]
    def cost(p):
        cx, cy, sx = p; r = np.sqrt(((xs - cx) * sx) ** 2 + (ys - cy) ** 2); b = r.astype(int)
        s = np.bincount(b, ls, 900); n = np.bincount(b, None, 900); m = s / np.maximum(n, 1)
        return np.minimum(np.abs(ls - m[b]), 30).mean()
    best = min(((cost((cx, cy, 1.0)), cx, cy) for cy in range(150, 411, 16) for cx in range(240, 401, 16)))
    res = optimize.minimize(cost, [best[1], best[2], 1.0], method='Nelder-Mead', options=dict(xatol=0.05, fatol=1e-4, initial_simplex=[[best[1], best[2], 1.0], [best[1] + 8, best[2], 1.0], [best[1], best[2] + 8, 1.0], [best[1], best[2], 0.95]]))
    return res.x, res.fun

def ring_model(img, strips_only=True):
    h, w, _ = img.shape
    x = np.mgrid[0:h, 0:w][1]
    mask = ((x < 110) | (x >= 530)) if strips_only else np.ones((h, w), bool)
    (cx, cy, sx), c = fit_rings(img, mask)
    r = radius(h, w, cx, cy, sx)
    prof, cnt = binned_median(img.reshape(h, w, 3), r, mask, 1000)
    return dict(cx=cx, cy=cy, sx=sx, prof=prof, cost=c)

def fill_profile(prof):
    p = prof.copy(); idx = np.arange(len(p)); ok = ~np.isnan(p[:, 0])
    for c in range(3): p[:, c] = np.interp(idx, idx[ok], p[ok, c])
    return p

def make_recolor(src_bg, ref_bg):
    """devolve função recolor(img, ox, oy) para imagens desenhadas sobre src_bg na posição (ox, oy)"""
    A = ring_model(src_bg, True); B = ring_model(ref_bg, False)
    pB = fill_profile(B['prof'])
    # mesmo desenho de anéis: a cor de A em cada raio vem do mapa de cores B->A (ajustado onde os dois foram medidos)
    okA = ~np.isnan(A['prof'][:, 0]); okA[:int(200 / BIN)] = False
    X = np.c_[pB[okA], np.ones(okA.sum())]; Y = A['prof'][okA]
    M, *_ = np.linalg.lstsq(X, Y, rcond=None)
    pA_pred = np.c_[pB, np.ones(len(pB))] @ M
    pA = np.where(okA[:, None], A['prof'], pA_pred)
    err = np.abs(pA_pred[okA] - A['prof'][okA]).mean()
    print('anel A centro %.2f,%.2f achat %.3f | B centro %.2f,%.2f achat %.3f | erro do mapa de cores %.2f' % (A['cx'], A['cy'], A['sx'], B['cx'], B['cy'], B['sx'], err))
    def recolor(img, ox=0, oy=0):
        img = img.astype(np.float64); h, w, _ = img.shape
        r0 = radius(h, w, A['cx'], A['cy'], A['sx'], ox, oy)
        best = None
        for dl in (0, -0.5, 0.5, -1, 1, -1.5, 1.5, -2, 2):      # tolera as bordas dos anéis (cor muda rápido)
            i = np.clip((r0 + dl) / BIN, 0, len(pA) - 2); i0 = i.astype(int); t = (i - i0)[..., None]
            a_ = pA[i0] * (1 - t) + pA[i0 + 1] * t; b_ = pB[i0] * (1 - t) + pB[i0 + 1] * t
            s_ = np.clip((img * a_).sum(-1) / np.maximum((a_ * a_).sum(-1), 1), 0, 2)
            d_ = np.abs(img - s_[..., None] * a_).max(-1) + abs(dl) * 1.5
            if best is None: a, b, s, d = a_, b_, s_, d_; best = 1
            else:
                k = d_ < d; a = np.where(k[..., None], a_, a); b = np.where(k[..., None], b_, b); s = np.where(k, s_, s); d = np.where(k, d_, d)
        wring = np.clip((16 - d) / 10, 0, 1) * np.clip((s - 0.25) / 0.15, 0, 1) * np.clip((1.12 - s) / 0.06, 0, 1)
        ring = wring > 0.99
        fg = (wring < 0.01); fg_core = ndimage.binary_erosion(fg, iterations=1)
        if fg_core.any():
            _, (iy, ix) = ndimage.distance_transform_edt(~fg_core, return_indices=True)
            c = img[iy, ix]
        else:
            c = img
        ca = c - a; alpha = np.clip(((img - a) * ca).sum(-1) / np.maximum((ca * ca).sum(-1), 1e-6), 0, 1)
        alpha = np.where(fg_core, 1.0, alpha)
        # brilhos e sombras semitransparentes (o contorno amarelo dos ícones): quanto mais perto da cor do anel,
        # mais transparente; o anel de baixo troca de cor e o brilho continua por cima
        alpha = np.minimum(alpha, np.clip(np.sqrt(((img - a) ** 2).sum(-1)) / 50.0, 0, 1))
        mixed = img + (1 - alpha)[..., None] * (b - a)
        out = wring[..., None] * (s[..., None] * b) + (1 - wring[..., None]) * mixed
        return np.clip(out + 0.5, 0, 255).astype(np.uint8)
    return recolor
