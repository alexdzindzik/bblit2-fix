"""Pacote HD do HUD: amplia 4x as texturas do HUD exportadas pelo mod (bin\\hd\\hud\\fonte\\<hash>.bmp).

Silhuetas (preto e branco, como as do Pernalonga e do Taz) são retraçadas em vetor e preenchidas com
antisserrilhado; ícones coloridos vão pelo Real-ESRGAN (o alfa à parte); degradês (barras) só bicúbico.
Nenhuma imagem do jogo vem no repositório: tudo sai das texturas do próprio jogo do jogador.
"""
import os, re, struct
import numpy as np
import cv2

SIMPLIFY = 0.6               # desvio máximo (px da textura) ao trocar os degraus por retas
CHAIKIN = 3                  # rodadas de arredondamento
CORNER = 100.0               # pontas mais finas que isso (graus de virada) não arredondam
ROUND = 1.5                  # raio máximo do arredondamento numa quina (px da textura)
GRAY_COVER = 0.45            # quanto um pixel cinza da borda conta como silhueta
OUTLINE = 1.0                # largura do contorno branco (px da textura)
FORCE_ESRGAN = set()          # hashes de silhuetas que ficam melhores pelo ESRGAN (preenchida depois da comparação)


# ---------------------------------------------------------------- BMP 32 bits (BGRA, de baixo para cima)
def read_bmp32(path):
    """-> [h, w, 4] uint8 RGBA, na ordem de uma imagem (linha 0 em cima)"""
    b = open(path, 'rb').read()
    if len(b) < 54 or b[:2] != b'BM':
        raise ValueError('não é BMP: %s' % path)
    off, = struct.unpack_from('<I', b, 10)
    w, h, planes, bpp, comp = struct.unpack_from('<iiHHI', b, 18)
    if bpp != 32 or comp not in (0, 3):
        raise ValueError('BMP precisa ser 32 bits: %s' % path)
    flip = h > 0; h = abs(h)
    n = w * h * 4
    if len(b) < off + n:
        raise ValueError('BMP truncado: %s' % path)
    px = np.frombuffer(b, np.uint8, n, off).reshape(h, w, 4)
    if flip:
        px = px[::-1]
    return px[..., [2, 1, 0, 3]].copy()


def write_bmp32(path, rgba):
    rgba = np.ascontiguousarray(rgba, np.uint8)
    h, w = rgba.shape[:2]
    data = rgba[::-1][..., [2, 1, 0, 3]].tobytes()
    head = struct.pack('<2sIHHI', b'BM', 54 + len(data), 0, 0, 54)
    info = struct.pack('<IiiHHIIiiII', 40, w, h, 1, 32, 0, len(data), 2835, 2835, 0, 0)
    d = os.path.dirname(path)
    if d:
        os.makedirs(d, exist_ok=True)
    with open(path, 'wb') as f:
        f.write(head + info + data)


# ---------------------------------------------------------------- tipo de textura
def classify(rgba):
    a = rgba[..., 3] > 0
    if not a.any():
        return 'icon'
    rgb = rgba[..., :3].astype(int)
    h, w = a.shape
    if h <= 16 and w >= 4 * h:                          # faixa fina: preenchimento de barra (pontas podem ser chanfradas)
        return 'gradient'
    px = rgb[a]
    bw = (px.max(1) < 70) | (px.min(1) > 190)          # preto ou branco
    if bw.mean() > 0.9 and (px.max(1) < 70).mean() > 0.2:
        return 'silhouette'
    return 'icon'


# ---------------------------------------------------------------- traçado das silhuetas
def _corners(p, limit=60.0):
    """vértices onde o contorno vira mais que `limit` graus (pontas finas, que não devem arredondar)"""
    v1 = p - np.roll(p, 1, 0)
    v2 = np.roll(p, -1, 0) - p
    c = (v1 * v2).sum(1) / (np.linalg.norm(v1, axis=1) * np.linalg.norm(v2, axis=1) + 1e-9)
    return np.degrees(np.arccos(np.clip(c, -1, 1))) > limit


def _chaikin(p, fixed, it=2):
    for _ in range(it):
        q, f = [], []
        n = len(p)
        for i in range(n):
            if fixed[i]:
                q.append(p[i]); f.append(True); continue
            a, b, c = p[i - 1], p[i], p[(i + 1) % n]
            q.append(0.25 * a + 0.75 * b); f.append(False)
            q.append(0.75 * b + 0.25 * c); f.append(False)
        p, fixed = np.array(q), np.array(f)
    return p


def _limit_round(p, r):
    """põe pontos a `r` px de cada vértice: o Chaikin só arredonda até ali (lado longo não vira curva)"""
    q = []
    n = len(p)
    for i in range(n):
        a, b = p[i], p[(i + 1) % n]
        q.append(a)
        d = np.linalg.norm(b - a)
        if d > 2 * r:
            u = (b - a) / d
            q.append(a + u * r); q.append(b - u * r)
    return np.array(q)


def _canvas(mask, hi):
    """máscara (0..1; valores do meio puxam o contorno para perto do pixel) -> polígonos suavizados em `hi`x (0/255).

    O contorno passa pelo meio das arestas dos pixels (marching squares em 0,5, pixels na diagonal ligados):
    degrau de 1 px vira reta e traço de 1 px continua com 1 px. Degraus rasos viram retas (Douglas-Peucker)
    e o Chaikin arredonda as quinas, menos as pontas finas."""
    from skimage import measure
    h, w = mask.shape
    canvas = np.zeros((h * hi, w * hi), np.uint8)
    if not (mask > 0.5).any():
        return canvas
    m = np.pad(mask.astype(np.float32), 1)
    sub = 4                                              # bits de fração do fillPoly
    tmp = np.zeros_like(canvas)
    for c in measure.find_contours(m, 0.5, fully_connected='high'):
        p = c[:-1, ::-1] if np.allclose(c[0], c[-1]) else c[:, ::-1]   # (x, y), sem o ponto repetido
        if len(p) > 4:                                   # degraus rasos (1:2, 1:3...) viram retas antes de arredondar
            q = cv2.approxPolyDP(p.astype(np.float32).reshape(-1, 1, 2), SIMPLIFY, True)[:, 0, :].astype(np.float64)
            a0 = abs(cv2.contourArea(p.astype(np.float32)))
            if len(q) >= 3 and abs(cv2.contourArea(q.astype(np.float32))) >= 0.8 * a0:
                p = q                                    # traço fino colapsaria (área some): fica o contorno inteiro
        if len(p) < 3:
            continue
        p = _limit_round(p, ROUND)                       # pontos novos ficam em reta: nunca são pontas
        p = _chaikin(p, _corners(p, CORNER), CHAIKIN)
        p = (p - 0.5) * hi - 0.5                         # índice com margem -> pixel da tela grande
        tmp[:] = 0
        cv2.fillPoly(tmp, [np.round(p * (1 << sub)).astype(np.int32).reshape(-1, 1, 2)], 255, cv2.LINE_8, sub)
        canvas ^= tmp                                    # par-ímpar: furos se abrem sozinhos
    return canvas


def trace_silhouette(rgba, scale=4):
    """silhueta preta com contorno branco -> `scale`x, lisa e com antisserrilhado.

    O preto é traçado; o contorno branco é o preto alargado por OUTLINE px (largura igual em toda a volta),
    somado ao traçado da silhueta inteira (partes brancas mais largas que isso). O cinza claro do original é
    o meio-tom da borda: conta como pixel parcial (GRAY_COVER), e o antisserrilhado da saída faz esse papel."""
    hi = scale * 4
    a = rgba[..., 3] > 0
    lum = rgba[..., :3].astype(np.float32) @ np.array([0.299, 0.587, 0.114], np.float32)
    black = a & (lum < 128)
    bk = _canvas(black, hi)
    out = cv2.dilate((~a).astype(np.uint8), cv2.getStructuringElement(cv2.MORPH_CROSS, (3, 3)), borderValue=1) > 0
    gray = a & (lum >= 128) & (lum <= 230) & out        # só o cinza da beirada; cinza cercado não abre furo
    al = _canvas(np.where(gray, GRAY_COVER, a.astype(np.float32)), hi)
    if black.any():
        d = cv2.distanceTransform(np.where(bk > 0, 0, 255).astype(np.uint8), cv2.DIST_L2, 5)
        al = np.maximum(al, np.where(d <= OUTLINE * hi, 255, 0).astype(np.uint8))
    bk = np.minimum(bk, al)
    h, w = a.shape
    size = (w * scale, h * scale)
    s = cv2.resize(al, size, interpolation=cv2.INTER_AREA).astype(np.float32) / 255.0
    b = cv2.resize(bk, size, interpolation=cv2.INTER_AREA).astype(np.float32) / 255.0
    white = np.where(s > 0, 1.0 - b / np.maximum(s, 1e-6), 1.0)
    out = np.zeros(s.shape + (4,), np.uint8)
    out[..., :3] = np.clip(white * 255 + 0.5, 0, 255).astype(np.uint8)[..., None]
    out[..., 3] = np.clip(s * 255 + 0.5, 0, 255).astype(np.uint8)
    return out


# ---------------------------------------------------------------- ícones e degradês
def _bleed(rgba):
    """RGB dos pixels transparentes = o do opaco mais próximo (no jogo eles são pretos e mancham a borda ao ampliar)"""
    from scipy import ndimage
    op = rgba[..., 3] > 0
    if op.all() or not op.any():
        return rgba[..., :3].copy()
    _, (iy, ix) = ndimage.distance_transform_edt(~op, return_indices=True)
    return rgba[iy, ix, :3]


def _upscale_alpha(al, scale):
    h, w = al.shape
    up = cv2.resize(al, (w * scale, h * scale), interpolation=cv2.INTER_CUBIC).astype(np.float32) / 255.0
    up = np.clip((up - 0.5) * (scale / 2.0) + 0.5, 0, 1)                 # recorte em 0,5 com rampa de ~2 px
    return np.clip(up * 255 + 0.5, 0, 255).astype(np.uint8)


def upscale_icon(sess, rgba, scale=4):
    from make_pack import esrgan
    up = esrgan(sess, _bleed(rgba))
    return np.dstack([up, _upscale_alpha(rgba[..., 3], scale)])


def upscale_gradient(rgba, scale=4):
    h, w = rgba.shape[:2]
    rgb = cv2.resize(np.ascontiguousarray(_bleed(rgba)), (w * scale, h * scale), interpolation=cv2.INTER_CUBIC)
    return np.dstack([rgb, _upscale_alpha(rgba[..., 3], scale)])


# ---------------------------------------------------------------- pacote
def _sheet(pairs, scale=4):
    """original (ampliado sem suavizar) ao lado do HD, um por linha, sobre xadrez; de cabeça para cima
    (a linha 0 da textura do OpenGL é a de baixo)"""
    rows = []
    for name, src, hd in pairs:
        src, hd = src[::-1], hd[::-1]
        big = cv2.resize(src, (src.shape[1] * scale, src.shape[0] * scale), interpolation=cv2.INTER_NEAREST)
        row = np.concatenate([big, np.zeros((big.shape[0], 16, 4), np.uint8), hd], 1)
        rows.append(row)
    if not rows:
        return None
    W = max(r.shape[1] for r in rows)
    rows = [np.pad(r, ((0, 16), (0, W - r.shape[1]), (0, 0))) for r in rows]
    img = np.concatenate(rows, 0).astype(np.float32)
    yy, xx = np.indices(img.shape[:2])
    chk = np.where(((yy // 16) + (xx // 16)) % 2, 70, 110).astype(np.float32)[..., None]
    a = img[..., 3:4] / 255.0
    rgb = img[..., :3] * a + chk * (1 - a)
    return np.clip(rgb + 0.5, 0, 255).astype(np.uint8)


def build_hud_pack(bin_dir, sess, log=print):
    src_dir = os.path.join(bin_dir, 'hd', 'hud', 'fonte')
    out_dir = os.path.join(bin_dir, 'hd', 'hud')
    if not os.path.isdir(src_dir):
        return 0
    n, pairs = 0, []
    for f in sorted(os.listdir(src_dir)):
        m = re.fullmatch(r'([0-9a-f]{16})\.bmp', f.lower())
        if not m:
            continue
        h = m.group(1)
        try:
            img = read_bmp32(os.path.join(src_dir, f))
        except ValueError as e:
            log('HUD: %s' % e); continue
        kind = classify(img)
        if kind == 'silhouette' and h not in FORCE_ESRGAN:
            hd = trace_silhouette(img, 4)
        elif kind == 'gradient':
            hd = upscale_gradient(img, 4)
        else:
            if sess is None:
                continue
            hd = upscale_icon(sess, img, 4)
            kind = 'icon'
        write_bmp32(os.path.join(out_dir, h + '.bmp'), hd)
        pairs.append((h, img, hd)); n += 1
        log('HUD %s %-10s %3dx%-3d -> %dx%d' % (h, kind, img.shape[1], img.shape[0], hd.shape[1], hd.shape[0]))
    sheet = _sheet(pairs)
    if sheet is not None:
        cv2.imwrite(os.path.join(out_dir, 'hud_comparacao.png'), sheet[..., ::-1])
    return n
