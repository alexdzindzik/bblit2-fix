"""Compara duas capturas BMP de 24 bits do BBLIT2 Fix (quadro do jogo x quadro extra).

Uso: python3 comparar.py captura.bmp captura_r.bmp
Mostra a diferença máxima por canal e quantos pixels são diferentes.
"""
import struct
import sys


def read_bmp(path):
    d = open(path, 'rb').read()
    off = struct.unpack_from('<I', d, 10)[0]
    w, h = struct.unpack_from('<ii', d, 18)
    bpp = struct.unpack_from('<H', d, 28)[0]
    if bpp != 24:
        raise SystemExit(f'{path}: esperado BMP de 24 bits, veio {bpp}')
    stride = (w * 3 + 3) & ~3
    return w, abs(h), stride, d[off:off + stride * abs(h)]


def main(a, b):
    wa, ha, sa, pa = read_bmp(a)
    wb, hb, sb, pb = read_bmp(b)
    if (wa, ha) != (wb, hb):
        raise SystemExit(f'tamanhos diferentes: {wa}x{ha} e {wb}x{hb}')
    maxd, ndiff = 0, 0
    for y in range(ha):
        ra, rb = pa[y * sa:y * sa + wa * 3], pb[y * sb:y * sb + wb * 3]
        if ra == rb:
            continue
        for x in range(0, wa * 3, 3):
            d = max(abs(ra[x + c] - rb[x + c]) for c in range(3))
            if d:
                ndiff += 1
                maxd = max(maxd, d)
    print(f'{wa}x{ha}: diferença máxima {maxd}, pixels diferentes {ndiff} ({100.0 * ndiff / (wa * ha):.3f}%)')
    return 0 if ndiff == 0 else 1


if __name__ == '__main__':
    if len(sys.argv) != 3:
        raise SystemExit(__doc__)
    sys.exit(main(sys.argv[1], sys.argv[2]))
