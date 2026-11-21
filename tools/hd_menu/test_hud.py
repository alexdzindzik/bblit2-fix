# Testes do pacote HD do HUD (rodam no Linux): python3 -m pytest tools/hd_menu/test_hud.py -q
import os, sys, tempfile
import numpy as np
import cv2

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import hud


def test_bmp32_roundtrip():
    rng = np.random.default_rng(1)
    img = rng.integers(0, 256, (3, 5, 4), dtype=np.uint8)
    img[0, 0, 3] = 0; img[1, 2, 3] = 128; img[2, 4, 3] = 255
    with tempfile.TemporaryDirectory() as d:
        p = os.path.join(d, 'x.bmp')
        hud.write_bmp32(p, img)
        back = hud.read_bmp32(p)
    assert back.shape == img.shape and back.dtype == np.uint8
    assert np.array_equal(back, img)


def _silhouette():
    """preto opaco com borda branca de 1 px, alfa 0/255 (como as silhuetas do HUD)"""
    a = np.zeros((32, 64, 4), np.uint8)
    cv2.ellipse(a, (32, 16), (24, 10), 0, 0, 360, (255, 255, 255, 255), -1)
    cv2.ellipse(a, (32, 16), (23, 9), 0, 0, 360, (0, 0, 0, 255), -1)
    return a


def test_classify():
    assert hud.classify(_silhouette()) == 'silhouette'
    g = np.zeros((8, 128, 4), np.uint8)
    g[..., 0] = np.linspace(255, 200, 128).astype(np.uint8)[None, :]
    g[..., 1] = np.linspace(40, 220, 128).astype(np.uint8)[None, :]
    g[..., 3] = 255
    assert hud.classify(g) == 'gradient'
    t = g.copy()                                          # como a barra do jogo: pontas chanfradas, alfa 0 nos cantos
    t[:2, :, 3] = 0
    for y, n in ((2, 3), (3, 2), (6, 2), (7, 3)):
        t[y, :n, 3] = 0; t[y, -n:, 3] = 0
    assert hud.classify(t) == 'gradient'
    c = np.zeros((32, 32, 4), np.uint8)
    cv2.circle(c, (16, 16), 13, (0, 0, 0, 255), -1)
    cv2.circle(c, (16, 16), 11, (255, 220, 30, 255), -1)
    assert hud.classify(c) == 'icon'


def test_trace_keeps_thin_lines():
    a = np.zeros((32, 32, 4), np.uint8)
    for i in range(32):                                   # diagonal preta de 1 px com contorno branco
        for dy in (-1, 0, 1):
            for dx in (-1, 0, 1):
                y, x = i + dy, i + dx
                if 0 <= y < 32 and 0 <= x < 32:
                    a[y, x] = (255, 255, 255, 255)
    for i in range(32):
        a[i, i] = (0, 0, 0, 255)
    out = hud.trace_silhouette(a, 4)
    assert out.shape == (128, 128, 4)
    for i in range(32):
        blk = out[i * 4:i * 4 + 4, i * 4:i * 4 + 4, 3]
        assert blk.max() >= 128, 'trecho %d da diagonal sumiu' % i


def test_trace_antialiased():
    a = np.zeros((16, 16, 4), np.uint8)
    a[2:14, 2:14] = (255, 255, 255, 255)
    a[3:13, 3:13] = (0, 0, 0, 255)
    out = hud.trace_silhouette(a, 4)
    al = out[..., 3].astype(int)
    mid = (al > 0) & (al < 255)
    assert mid.any(), 'sem meio-tom: borda serrilhada'
    edge = np.zeros((64, 64), bool)                       # borda do quadrado ampliado (8..56)
    edge[6:58, 6:58] = True
    edge[10:54, 10:54] = False
    assert np.all(edge[mid]), 'meio-tom longe da borda'


def test_gradient_no_dark_fringe():
    """pixels transparentes são pretos no jogo; ampliar não pode escurecer a borda da barra"""
    t = np.zeros((8, 128, 4), np.uint8)
    t[2:6, 2:126] = (255, 200, 40, 255)
    out = hud.upscale_gradient(t, 4)
    assert out.shape == (32, 512, 4)
    edge = out[..., 3] > 0
    assert out[..., 0][edge].min() >= 240, 'borda escureceu (preto do fundo vazou)'


def test_trace_straightens_stairs():
    """escada de 1 px a 45 graus vira reta (nada de degrau de 4 px na saída 4x)"""
    a = np.zeros((32, 32, 4), np.uint8)
    for y in range(32):
        a[y, :y + 1] = (255, 255, 255, 255)
    out = hud.trace_silhouette(a, 4)
    area = out[..., 3].astype(np.float64).sum(1) / 255.0  # largura coberta em cada linha
    y4 = np.arange(128)[12:116]
    area = area[12:116]
    k, b = np.polyfit(y4, area, 1)
    res = np.abs(area - (k * y4 + b))
    assert res.max() <= 0.6, 'degrau de %.1f px' % res.max()
    ideal = y4 + 2.5                                      # reta pelos meios dos degraus
    assert np.abs(area - ideal).max() <= 2.0, 'borda saiu do lugar'   # até meio pixel da textura


def test_trace_keeps_thin_black_line():
    """traço preto de 1 px (perna, dedo) não pode sumir ao simplificar o contorno"""
    a = np.zeros((16, 32, 4), np.uint8)
    a[5:10, 2:30] = (255, 255, 255, 255)
    a[7, 4:28] = (0, 0, 0, 255)
    out = hud.trace_silhouette(a, 4)
    band = out[26:34, 24:104]                             # linhas do traço, longe das pontas
    darkest = band[..., :3].min(axis=(0, 2))
    assert darkest.max() <= 100, 'traço preto sumiu em alguma coluna'


def test_trace_keeps_white_dot():
    """olho/dente: 1 px branco dentro do preto continua aparecendo"""
    a = np.zeros((16, 16, 4), np.uint8)
    a[1:15, 1:15] = (255, 255, 255, 255)
    a[2:14, 2:14] = (0, 0, 0, 255)
    a[7, 7] = (255, 255, 255, 255)
    out = hud.trace_silhouette(a, 4)
    assert out[28:32, 28:32, :3].max() >= 160, 'ponto branco sumiu'
