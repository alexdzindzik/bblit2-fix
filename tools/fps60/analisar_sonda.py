"""Analisa bin\\fps_probe_N.bin, gravado com debug=1 e F5 (medição dos 60 fps do BBLIT2 Fix).

Uso: python3 analisar_sonda.py fps_probe_1.bin [mais arquivos]
"""
import struct, sys, collections, statistics as st

REC = struct.Struct('<BBHIII8fI')
NAMES = {1: 'vtx', 2: 'begin', 3: 'end', 4: 'ortho', 5: 'viewport', 6: 'clear', 7: 'bind', 8: 'enable', 9: 'disable', 10: 'swap'}


def load(path):
    d = open(path, 'rb').read()
    hdr = struct.unpack_from('<10I', d, 0)
    assert hdr[0] == 0x31535046, 'arquivo nao e FPS1'
    n, size = hdr[2], hdr[3]
    assert size == REC.size, (size, REC.size)
    recs = [REC.unpack_from(d, 40 + i * size) for i in range(n)]
    return hdr, recs


def pct(a, b):
    return f'{100.0 * a / b:5.1f}%' if b else '  -  '


def q(v, p):
    v = sorted(v)
    return v[min(len(v) - 1, int(p * len(v)))] if v else float('nan')


def analyse(path):
    hdr, recs = load(path)
    print(f'== {path}: {hdr[2]} registros, {hdr[4]} quadros, jogo {hdr[5]}x{hdr[6]}, janela {hdr[7]}x{hdr[8]}, perdidos {hdr[9]}')
    frames = collections.defaultdict(list)
    for r in recs:
        frames[r[3]].append(r)
    verts = {}
    for k in sorted(frames):
        fr = frames[k]
        cnt = collections.Counter(NAMES.get(r[0], r[0]) for r in fr)
        sw = [r for r in fr if r[0] == 10]
        t = ''
        if sw:
            qpc = struct.unpack('<q', struct.pack('<2f', *sw[0][6:8]))[0]
            fq = struct.unpack('<q', struct.pack('<2f', *sw[0][8:10]))[0]
            t = f' t={qpc / fq * 1000:.1f} ms contador={sw[0][4]} pulos={sw[0][5]}'
        orthos = [tuple(round(x, 1) for x in r[6:12]) for r in fr if r[0] == 4]
        clears = [hex(r[4]) for r in fr if r[0] == 6]
        begins = collections.Counter((r[1], hex(r[5])) for r in fr if r[0] == 2)
        print(f' quadro {k}: {dict(cnt)}{t}')
        print(f'   ortho {orthos[:4]} clear {clears} begin(modo,quem) {dict(begins)}')
        vs = [r for r in fr if r[0] == 1]
        verts[k] = vs
        ptrs = [r[4] for r in vs if r[4]]
        up = sorted(set(ptrs))
        print(f'   vertices {len(vs)}, com ponteiro {len(ptrs)}, ponteiros unicos {len(up)}')
        if up:
            pages = collections.Counter(p >> 20 for p in up)
            print('   regioes (MB):', ', '.join(f'{p << 20:#010x}:{c}' for p, c in sorted(pages.items())))
            diffs = collections.Counter(b - a for a, b in zip(up, up[1:]) if b - a < 256)
            print('   passos entre ponteiros:', diffs.most_common(6))
        callers = collections.Counter(hex(r[5]) for r in vs)
        print('   quem chamou glVertex:', callers.most_common(8))
    ks = sorted(verts)
    print(' --- casamento por ponteiro entre quadros seguidos ---')
    for a, b in zip(ks, ks[1:]):
        A = {}
        for r in verts[a]:
            if r[4]:
                A.setdefault(r[4], r)
        B = {}
        for r in verts[b]:
            if r[4]:
                B.setdefault(r[4], r)
        common = [p for p in B if p in A]
        same_tex = [p for p in common if A[p][2] == B[p][2]]
        same_uv = [p for p in same_tex if abs(A[p][9] - B[p][9]) < 1e-4 and abs(A[p][10] - B[p][10]) < 1e-4]
        d = [((B[p][6] - A[p][6]) ** 2 + (B[p][7] - A[p][7]) ** 2) ** .5 for p in same_uv]
        dz = [abs(B[p][8] - A[p][8]) for p in same_uv]
        print(f'  {a}->{b}: ponteiros em B {len(B)}, tambem em A {pct(len(common), len(B))}, mesma textura {pct(len(same_tex), len(common))}, mesmo s,t {pct(len(same_uv), len(common))}')
        if d:
            print(f'     deslocamento na tela (mesmo s,t): mediana {q(d, .5):.1f}  p90 {q(d, .9):.1f}  p99 {q(d, .99):.1f}  max {max(d):.1f};  |dz| mediana {q(dz, .5):.2f} p99 {q(dz, .99):.2f}')
        # chave alternativa: (textura, s, t, quem chamou) — vale quando o ponteiro não é estável
        ka = collections.Counter((r[2], round(r[9], 4), round(r[10], 4)) for r in verts[a])
        kb = collections.Counter((r[2], round(r[9], 4), round(r[10], 4)) for r in verts[b])
        uniq_b = [k for k, c in kb.items() if c == 1]
        print(f'     chave (textura,s,t): {len(kb)} chaves em B, unicas {pct(len(uniq_b), len(kb))}, tambem em A {pct(sum(1 for k in kb if k in ka), len(kb))}')


if __name__ == '__main__':
    for p in sys.argv[1:]:
        analyse(p)
