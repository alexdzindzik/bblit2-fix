# Histórico de versões

## 0.13.1
- **Andar analógico como no PS1** (`[pad] analog_joystick=1`): inclinar pouco anda devagar, inclinar tudo corre, em qualquer direção. O mod oferece ao jogo um joystick do DirectInput que lê o controle XInput (o Windows 10/11 quase nunca lista um para o controle Xbox) e acerta o limite em que o jogo passa a correr, que era bem menor para a direita e para baixo.
- **O amigo no analógico direito**, como no PS1 (`[pad] right_stick_partner=1`; a câmera fica no LT/RT).
- **Brilho por gama** (`[postfx] brightness`, 100 = original): acima de 100 clareia os tons médios sem manchar as cores, abaixo escurece.
- Tudo desligado por padrão. Contribuição de [LeLordWHO](https://github.com/LeLordWHO) (PR #1), testada com controle Xbox no Windows 11.

## 0.13
- **HUD da fase em HD (opcional)**: o mod salva as texturas do HUD em `bin\hd\hud\fonte` e usa a versão 4× de `bin\hd\hud` gerada pelo `tools/hd_menu/make_pack.py`. As silhuetas do Pernalonga e do Taz são retraçadas em curvas (sem serrilhado, mantendo olhos, dedos e pernas); números, espelho, medalhas e setas passam pelo Real-ESRGAN; os degradês das barras são ampliados. Sem o pacote, continua o xBR (`hud_hd`).
- **Sombra suave** atrás das silhuetas e dos ícones do HUD, para destacá-los do cenário (`[hud] shadow=45`; 0 desliga).
- O `make_pack.py` gera também a folha `hud_comparacao.png` (original × HD) e passa a precisar do `opencv-python` e do `scikit-image`.
- Se o Brilho do menu Vídeo mudar, o mod exporta as texturas do HUD de novo e avisa no log para rodar o script outra vez.
- As barras de vida continuam com a animação do próprio jogo (ele já desce e sobe a barra aos poucos).

## 0.12.2
- **Correção**: com o controle conectado, as dicas não mostravam Ⓐ Ⓑ Ⓧ Ⓨ ("PARA AJUDA." sem o botão). O mod usava caracteres que não existem na fonte do jogo; agora usa os do PlayStation que existem (`0xac`–`0xaf`).
- **Ícones do controle redondos**, todos do mesmo tamanho e na altura das letras (o jogo desenha cada tipo de ícone num quadro de proporção diferente).
- **B volta ao menu anterior** nos menus da pausa, e a dica mostra Ⓑ. O View continua funcionando.
- **Seta do mouse some** enquanto o controle navega nos menus do início e volta quando o mouse se mexe.
- **Menus "nome ... valor" centralizados** (Opções > Som etc.), que em português ficavam deslocados para a esquerda (`center_menus`).
- **Correção**: com `object_distance` alto, o Piu-Piu deixava de aparecer onde o Pernalonga estava. Gatilhos e pontos invisíveis (objetos de lógica, sem modelo) voltam ao alcance original.
- **Problema conhecido**: possivelmente o Piu-Piu ainda pode desaparecer e reaparecer (bug conhecido).
- **Diagnóstico** (`debug=1`): F9 em 4 passos (alcance e LOD separados), F6/F7 para o alcance por tipo de objeto e cópia pequena da captura em `capturas\ultima.bmp`; `[debug] hud_probe` registra como o jogo desenha o HUD e os textos (preparação da 0.13).

## 0.12.1
- **Correção**: com o pacote HD dos menus instalado e `hd_textures=0`, o jogo fechava ao entrar na fase. O contador de imagens HD dos menus tinha o mesmo nome da lista de texturas HD e somava nela. A compilação agora avisa declarações repetidas (`-Wredundant-decls`).
- **F12 tira uma captura** da imagem do jogo na resolução da janela, sem perdas, em `bin\capturas\` (BMP), já com antisserrilhado e efeitos. Só com o jogo em foco.
- **Diagnóstico**: se o jogo fechar com erro, o log mostra o módulo, o deslocamento, os registradores e os endereços da pilha.
- README: link para o teaser no YouTube.

## 0.12
- **Menus em 16:9 de verdade**: as telas 2D (inicial, configuração, carregar e aberturas) ocupam a tela inteira sem esticar. Cada fundo do jogo é medido assim que o jogo carrega a imagem e vira uma tela 16:9 pronta (os anéis continuam até as bordas; nas aberturas, a cor lisa do fundo). Ao trocar de menu, a tela inteira aparece de uma vez, sem as laterais preenchendo depois (`menu_fill=3`, novo padrão).
- **Pacote HD dos menus (opcional)**: `tools/hd_menu/make_pack.py` gera a pasta `bin\hd` com os ícones, logos e fundos dos menus ampliados 4× pelo Real-ESRGAN (anime 6B, via onnxruntime) a partir dos arquivos do jogo. A tela inicial pode ficar no azul escuro da tela de configuração (recolorindo também o fundo desenhado nos ícones dela). O mod mistura cada ícone ao fundo pronto e mede o perfil dos anéis na própria imagem HD (`menu_hd=1`).
- Nomes dos ícones que se encostavam em português ("Carregar Jogo" e "Configuração") são desenhados um pouco mais estreitos, no mesmo lugar.
- Compilação com SSE2.

## 0.11
- **Controle nos menus do início**: tela inicial, Carregar jogo e Configuração funcionam com o controle (antes só com o mouse). Direcional ou analógico pula de item em item, A/Start confirma, B volta; ao escolher um jogo salvo, o foco vai para "Carregar" (`menu_navigation`).

## 0.10
- **Letras e HUD em HD**: fonte, números, ícones e desenhos da pausa redesenhados em 4× com xBR, gerados na hora a partir das texturas do jogo (`hd_textures`).
- **Botões do Xbox nas dicas**: com o controle conectado, o jogo mostra Ⓐ Ⓑ Ⓧ Ⓨ, LB, RB, BACK e Menu no lugar das teclas; ao desconectar, as teclas voltam na hora (`button_prompts`).
- **Menus 2D em tela cheia**: laterais com fundo desfocado (`menu_fill`) e textos na resolução do monitor (`hires_menu`).
- **Correção**: as telas Vídeo e Controles do menu mostravam só um item.
- **Correção**: com o "Brilho" do menu Vídeo diferente de quando a lista foi feita, a fonte HD e os ícones do controle não apareciam (as texturas eram reconhecidas pelas cores). Agora são reconhecidas pelo desenho, com qualquer ajuste de cor.
- **Correção**: texturas de personagens podiam herdar a marca HD de uma textura antiga com o mesmo número (o jogo reaproveita os números). Agora as marcas são esquecidas quando a textura é apagada ou reenviada, e cada atualização parcial confere o tamanho real.
- Texturas de efeitos e cenário que tinham entrado na lista HD por engano foram tiradas (só fonte e HUD).
- **Diagnóstico**: com `debug=1`, F8 finge controle desconectado/conectado; `debug=2` registra cada textura; `dump_textures` grava a assinatura usada pela lista.

## 0.9
- **LOD dos modelos**: personagens e objetos mantêm o modelo detalhado até 4× mais longe antes de trocar pelo simplificado (`lod_distance`).
- **Pós-processamento opcional** (`[postfx]`): bloom, nitidez adaptativa (CAS), saturação, contraste e vinheta, em shaders GLSL aplicados à imagem interna.
- **HUD alinhado no widescreen**: os ícones 3D (cenoura, moeda de ouro) voltam para junto dos contadores, no 4:3 central.
- **Compatibilidade**:
  - Antisserrilhado e efeitos tentam configurações mais leves se a placa recusar (memória, limites, falta de shaders).
  - Monitores 4K ficam sem supersampling.
  - Os objetos de vídeo são liberados ao trocar de configuração.
  - Mipmaps e anisotropia só são usados se a placa suportar.
- **`config.pc` ajustado em memória** logo depois que o jogo o lê: a resolução certa já vale na primeira execução, e o renderizador volta para OpenGL se o jogo tiver caído para Direct3D/software (`force_opengl`).
- **Pasta portátil**: o jogo passa a usar a pasta de onde está rodando (`config.pc` e saves na própria `bin`), mesmo que o registro aponte para outra instalação.
- **Diagnóstico**: com `debug=1`, F9 alterna entre as distâncias originais e as do mod, e F11 liga e desliga os efeitos.

## 0.8
- **Itens e personagens aparecem de mais longe**: o alcance de ativação dos objetos é multiplicado (`object_distance`, padrão 200%).
- **Widescreen sem objetos sumindo nas laterais**: os três testes de frustum da lista de visão foram corrigidos para a tela larga.
- **Reconhecimento estrito do executável** (CRC32 do `bblit2.exe` brasileiro). Em outras versões, o mod se desliga sozinho.
- **Funciona sem o jogo instalado**: se a chave do registro não existir, a DLL responde com a pasta do jogo.
- **Qualidade automática** (`auto_quality`): se o PC não segurar 30 fps durante o jogo, o antisserrilhado é reduzido.
- Monitores 5:4 ficam em 4:3, com faixas em cima e embaixo.

## 0.7 e anteriores
- Proxy `dinput.dll`: correções em memória, nenhum arquivo do jogo alterado.
- Tela cheia na resolução do monitor, sem trocar o modo de vídeo e sem exigir área de trabalho em 16 bits.
- Widescreen Hor+, com menus 2D em 4:3 ampliados e mouse mapeado.
- Antisserrilhado por FBO (MSAA 4× + supersampling 2×), mipmaps e filtro anisotrópico 16×.
- Distância de visão até 4×, com buffers do motor 4× maiores.
- Controle XInput somado ao teclado do jogo, com câmera no analógico direito e inversão configurável.
- Idioma e falas em português detectados pelo CD.
- Jogo continua rodando sem foco (Alt+Tab e outro monitor).
