# Pacote HD dos menus (opcional)

Gera a pasta `bin\hd` que o `dinput.dll` do BBLIT2 Fix usa nos menus 2D:

- **ícones, logos e fundos ampliados 4×** com o [Real-ESRGAN](https://github.com/xinntao/Real-ESRGAN) (modelo `RealESRGAN_x4plus_anime_6B`, licença BSD-3), rodando pelo `onnxruntime`, sem PyTorch;
- **tela inicial em azul escuro**: os anéis azul-claro da tela inicial (e o pedaço de fundo que vem desenhado nos ícones dela) ganham as cores dos anéis da tela de configuração. As sombras e os brilhos continuam por cima. Para manter o azul original, use `--sem-tela-escura`.

![tela inicial com o pacote HD](../../docs/menu_hd.jpg)

O mod mistura cada ícone HD com o fundo já pronto, então não aparece o retângulo do ícone. As laterais 16:9 são desenhadas com os anéis medidos na própria imagem HD.

## Como gerar

```
pip install numpy scipy pillow onnx onnxruntime
python make_pack.py "C:\Jogos\BBLIT2\data" "C:\Jogos\BBLIT2\bin"
```

Na primeira vez o script baixa o modelo (18 MB) do GitHub do Real-ESRGAN e converte para ONNX na pasta `cache`. Leva uns 5 minutos na CPU e ocupa cerca de 100 MB.

Arquivos gerados:

| Arquivo | O que é |
|---|---|
| `bin\hd\<nome>.bmp` | a imagem 4× |
| `bin\hd\1x\<nome>.bmp` | versão 1× que substitui a original (só a tela inicial e os ícones dela, quando recoloridos) |

Para desligar sem apagar a pasta: `menu_hd=0` em `[video]` no `bblit2fix.ini`.

**As imagens geradas são do jogo: use no seu computador e não distribua o pacote.** Este repositório tem só os scripts.
