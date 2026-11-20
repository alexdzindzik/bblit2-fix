#!/usr/bin/env python3
"""BBLIT2 Fix - pacote de imagens HD dos menus (opcional).

Amplia 4x as imagens dos menus do jogo (pasta data) com o Real-ESRGAN (modelo anime 6B, rodando pelo
onnxruntime) e grava em bin\\hd, de onde o dinput.dll do mod lê. Com --tela-escura (padrão), a tela inicial
e os ícones dela ganham as cores escuras dos anéis da tela de configuração.

Uso:  python make_pack.py <pasta data do jogo> <pasta bin do jogo>
Requer: pip install numpy scipy pillow onnx onnxruntime
As imagens geradas são do jogo: não distribua o pacote.
"""
import argparse, os, re, sys, time, urllib.request
import numpy as np
from PIL import Image
from scipy import ndimage
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from recolor import make_recolor, ring_model, radius, fill_profile, BIN

MODEL_URL = 'https://github.com/xinntao/Real-ESRGAN/releases/download/v0.2.2.4/RealESRGAN_x4plus_anime_6B.pth'

def get_session(cache):
    import onnxruntime as ort
    onnx_path = os.path.join(cache, 'anime6b.onnx')
    if not os.path.exists(onnx_path):
        pth = os.path.join(cache, 'RealESRGAN_x4plus_anime_6B.pth')
        if not os.path.exists(pth):
            print('baixando o modelo do Real-ESRGAN...'); urllib.request.urlretrieve(MODEL_URL, pth)
        import subprocess
        subprocess.check_call([sys.executable, os.path.join(os.path.dirname(os.path.abspath(__file__)), 'build_onnx.py'), pth, onnx_path])
    return ort.InferenceSession(onnx_path, providers=['CPUExecutionProvider'])

def esrgan(sess, a, tile=160, pad=12):
    """a: HxWx3 uint8 -> 4H x 4W x 3 uint8 (em blocos, com margem para não marcar as emendas)"""
    h, w, _ = a.shape
    src = np.pad(a.astype(np.float32) / 255.0, ((pad, pad), (pad, pad), (0, 0)), mode='reflect')
    out = np.zeros((h * 4, w * 4, 3), np.float32)
    for y0 in range(0, h, tile):
        for x0 in range(0, w, tile):
            y1, x1 = min(h, y0 + tile), min(w, x0 + tile)
            blk = src[y0:y1 + 2 * pad, x0:x1 + 2 * pad].transpose(2, 0, 1)[None]
            r = sess.run(None, {'input': blk})[0][0].transpose(1, 2, 0)
            out[y0 * 4:y1 * 4, x0 * 4:x1 * 4] = r[pad * 4:pad * 4 + (y1 - y0) * 4, pad * 4:pad * 4 + (x1 - x0) * 4]
    return (np.clip(out, 0, 1) * 255 + 0.5).astype(np.uint8)

def fg_mask_background(a):
    """o que está desenhado por cima do fundo (anéis ou cor lisa): 0..1"""
    a = a.astype(np.float64); h, w, _ = a.shape
    edge = np.concatenate([a[:, :6].reshape(-1, 3), a[:, -6:].reshape(-1, 3)])
    if (edge @ [0.299, 0.587, 0.114]).std() < 3:                       # cor lisa (aberturas)
        d = np.abs(a - np.median(edge, axis=0)).max(-1)
    else:
        m = ring_model(a, True)
        if m['cost'] > 4: return np.ones((h, w))                       # fundo desconhecido: tudo HD
        p = fill_profile(m['prof']); r = radius(h, w, m['cx'], m['cy'], m['sx'])
        d = np.full((h, w), 1e9)
        for dl in (0, -1, 1, -2, 2):
            i = np.clip((r + dl) / BIN, 0, len(p) - 2); i0 = i.astype(int); t = (i - i0)[..., None]
            d = np.minimum(d, np.abs(a - (p[i0] * (1 - t) + p[i0 + 1] * t)).max(-1) + abs(dl) * 2)
    fg = d > 10
    fg = ndimage.binary_opening(fg, iterations=1)
    fg = ndimage.binary_dilation(fg, iterations=2)
    return ndimage.gaussian_filter(fg.astype(np.float64), 1.0)

def save_bmp(img, path):
    os.makedirs(os.path.dirname(path), exist_ok=True)
    Image.fromarray(img).save(path, format='BMP')

def parse_menu(path):
    txt = open(path, 'rb').read().decode('latin-1')
    pages = re.split(r'\n(?=\[PAGE_\d+\])', txt); out = []
    for pg in pages:
        m = re.match(r'\[PAGE_(\d+)\]', pg.strip())
        if not m: continue
        items = []
        for sec in re.split(r'\n(?=\[)', pg):
            d = dict(l.strip().split('=', 1) for l in sec.split('\n') if '=' in l and not l.strip().startswith('//'))
            items.append(d)
        out.append((int(m.group(1)), items))
    return out

def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('data'); ap.add_argument('bin')
    ap.add_argument('--sem-tela-escura', action='store_true', help='mantém o azul-claro original da tela inicial')
    ap.add_argument('--so', nargs='*', help='processa só estes arquivos')
    ap.add_argument('--cache', default=os.path.join(os.path.dirname(os.path.abspath(__file__)), 'cache'))
    a = ap.parse_args()
    os.makedirs(a.cache, exist_ok=True)
    files = {f.lower(): f for f in os.listdir(a.data) if f.lower().endswith('.bmp')}
    load = lambda n: np.asarray(Image.open(os.path.join(a.data, files[n.lower()])).convert('RGB'))
    pages = parse_menu(os.path.join(a.data, 'menu.men'))
    # fundo de cada página (primeiro item: Up / Up0..Up6) e ícones com posição
    title_files, title_bg = {}, []
    page_bgs = {}
    for num, items in pages:
        bgs = [v for k, v in items[0].items() if re.fullmatch(r'Up\d?', k) and v.lower() in files]
        for b in bgs: page_bgs.setdefault(b.lower(), []).append(num)
        if num == 0:
            title_bg = [b.lower() for b in bgs]
            tb = load(title_bg[0]).astype(int) if title_bg else None
            for it in items[1:]:
                if 'x' in it and 'y' in it and tb is not None:
                    x, y = int(it['x']), int(it['y'])
                    for k in ('Up', 'Down'):
                        f = it.get(k, '').lower()
                        if f not in files: continue
                        ic = load(f).astype(int); h, w = ic.shape[:2]
                        # só os ícones desenhados com o pedaço da tela inicial atrás (os outros ficam como estão)
                        if y + h <= 480 and x + w <= 640 and (np.abs(ic - tb[y:y + h, x:x + w]).max(-1) <= 6).mean() > 0.25:
                            title_files[f] = (x, y)
    recolor = None
    if not a.sem_tela_escura and title_bg:
        others = [b for b in page_bgs if b not in title_bg and load(b).shape[:2] == (480, 640)]
        if others:
            ref = max(others, key=lambda b: len(page_bgs[b]))           # o fundo usado por mais telas (configuração)
            print('tela inicial com as cores de', files[ref])
            recolor = make_recolor(load(title_bg[0]).astype(float), load(ref).astype(float))
    sess = get_session(a.cache)
    hd = os.path.join(a.bin, 'hd')
    for n in sorted(files):
        t0 = time.time(); img = load(n)
        if n == 'pc02.bmp' or img.shape[0] * img.shape[1] < 64: continue
        if a.so and n not in [x.lower() for x in a.so]: continue
        if recolor and (n in title_bg or n in title_files):
            ox, oy = (0, 0) if n in title_bg else title_files[n]
            img = recolor(img, ox, oy); save_bmp(img, os.path.join(hd, '1x', files[n]))
        up = esrgan(sess, img)
        if img.shape[:2] == (480, 640):
            fg = fg_mask_background(img)
            if fg.mean() < 0.01: print('%-18s só anéis, sem HD' % files[n]); continue
            big = np.asarray(Image.fromarray(img).resize((2560, 1920), Image.BICUBIC)).astype(np.float64)
            m4 = np.asarray(Image.fromarray((fg * 255).astype(np.uint8)).resize((2560, 1920), Image.BILINEAR)).astype(np.float64)[..., None] / 255
            up = (big * (1 - m4) + up * m4 + 0.5).astype(np.uint8)
        save_bmp(up, os.path.join(hd, files[n]))
        print('%-18s %4dx%-4d -> %dx%d  %.1fs' % (files[n], img.shape[1], img.shape[0], up.shape[1], up.shape[0], time.time() - t0))

if __name__ == '__main__':
    main()
