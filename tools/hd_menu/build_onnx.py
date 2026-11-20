# Converte o RealESRGAN_x4plus_anime_6B.pth (RRDBNet) para ONNX sem precisar do PyTorch
import zipfile, pickle, sys, numpy as np, collections
import onnx
from onnx import helper, TensorProto, numpy_helper

def load_pth(path):
    z = zipfile.ZipFile(path)
    root = z.namelist()[0].split('/')[0]
    class Storage:
        def __init__(self, key, dtype): self.key, self.dtype = key, dtype
    def rebuild(storage, offset, size, stride, *a):
        raw = np.frombuffer(z.read(f'{root}/data/{storage.key}'), dtype=storage.dtype)
        n = int(np.prod(size)) if len(size) else 1
        arr = np.lib.stride_tricks.as_strided(raw[offset:], shape=size, strides=[s * raw.itemsize for s in stride]) if len(size) else raw[offset:offset+1].reshape(())
        return np.array(arr)
    dt = {'FloatStorage': np.float32, 'HalfStorage': np.float16, 'LongStorage': np.int64, 'IntStorage': np.int32}
    class U(pickle.Unpickler):
        def find_class(self, mod, name):
            if name == '_rebuild_tensor_v2': return rebuild
            if name in dt: return name
            if mod == 'collections' and name == 'OrderedDict': return collections.OrderedDict
            return super().find_class(mod, name)
        def persistent_load(self, pid):
            _, typ, key, loc, n = pid
            return Storage(key, dt[typ] if isinstance(typ, str) else np.float32)
    return U(z.open(f'{root}/data.pkl')).load()

sd = load_pth(sys.argv[1])
sd = sd.get('params_ema', sd.get('params', sd))
nblock = 1 + max(int(k.split('.')[1]) for k in sd if k.startswith('body.'))
print('blocks', nblock)
nodes, inits = [], []
cnt = [0]
def nm(p): cnt[0] += 1; return f'{p}_{cnt[0]}'
def conv(x, key):
    w = sd[key + '.weight'].astype(np.float32); b = sd[key + '.bias'].astype(np.float32)
    inits.append(numpy_helper.from_array(w, key + '.weight')); inits.append(numpy_helper.from_array(b, key + '.bias'))
    o = nm('conv'); nodes.append(helper.make_node('Conv', [x, key + '.weight', key + '.bias'], [o], kernel_shape=[3, 3], pads=[1, 1, 1, 1])); return o
def lrelu(x): o = nm('lr'); nodes.append(helper.make_node('LeakyRelu', [x], [o], alpha=0.2)); return o
def cat(xs): o = nm('cat'); nodes.append(helper.make_node('Concat', xs, [o], axis=1)); return o
c02 = numpy_helper.from_array(np.array(0.2, dtype=np.float32), 'c02'); inits.append(c02)
def axpy(a, b):  # a*0.2 + b
    m = nm('mul'); nodes.append(helper.make_node('Mul', [a, 'c02'], [m])); o = nm('add'); nodes.append(helper.make_node('Add', [m, b], [o])); return o
def add(a, b): o = nm('add'); nodes.append(helper.make_node('Add', [a, b], [o])); return o
inits.append(numpy_helper.from_array(np.array([1, 1, 2, 2], dtype=np.float32), 'sc2'))
def up2(x): o = nm('up'); nodes.append(helper.make_node('Resize', [x, '', 'sc2'], [o], mode='nearest')); return o
def rdb(x, k):
    x1 = lrelu(conv(x, k + '.conv1')); x2 = lrelu(conv(cat([x, x1]), k + '.conv2')); x3 = lrelu(conv(cat([x, x1, x2]), k + '.conv3'))
    x4 = lrelu(conv(cat([x, x1, x2, x3]), k + '.conv4')); x5 = conv(cat([x, x1, x2, x3, x4]), k + '.conv5'); return axpy(x5, x)
f = conv('input', 'conv_first'); b = f
for i in range(nblock):
    o = b
    for r in (1, 2, 3): o = rdb(o, f'body.{i}.rdb{r}')
    b = axpy(o, b)
f = add(f, conv(b, 'conv_body'))
f = lrelu(conv(up2(f), 'conv_up1')); f = lrelu(conv(up2(f), 'conv_up2'))
out = conv(lrelu(conv(f, 'conv_hr')), 'conv_last')
nodes.append(helper.make_node('Identity', [out], ['output']))
g = helper.make_graph(nodes, 'rrdb', [helper.make_tensor_value_info('input', TensorProto.FLOAT, [1, 3, None, None])],
                      [helper.make_tensor_value_info('output', TensorProto.FLOAT, [1, 3, None, None])], inits)
m = helper.make_model(g, opset_imports=[helper.make_opsetid('', 13)]); m.ir_version = 8
onnx.checker.check_model(m); onnx.save(m, sys.argv[2]); print('ok', sys.argv[2])
