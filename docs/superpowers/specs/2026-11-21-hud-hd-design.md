# HUD em HD com acabamento moderno (BBLIT2 Fix 0.13)

Data: 2026-11-21 · Escolha do Night: "fiel com acabamento moderno" (opção 2)

## Intenção

Dar ao *Furacão do Tempo* cara de jogo atual durante a partida. O HUD é o que mais denuncia a idade do jogo em 1440p: as silhuetas do Pernalonga e do Taz ao lado das barras de vida, o espelho, o relógio, os números amarelos, as medalhas e as setas são pixel art de 16 a 256 px, ampliada até ~3× na tela. Hoje o mod passa essas texturas pelo xBR 4×, o que tira a "escadinha" mas ainda deixa os contornos quadrados e come detalhe.

O resultado esperado: a mesma arte do jogo, nítida em qualquer resolução, com sombra suave por trás e barras de vida que se movem em vez de pular.

## Metas

1. **Ícones do HUD em HD de verdade**, gerados no PC do jogador a partir das texturas do próprio jogo (como o pacote HD dos menus).
2. **Sombra suave** atrás dos ícones 2D do HUD, para destacá-los do cenário.
3. **Barra de vida animada**: ao perder vida, a barra cai na hora e um rastro mais claro desce atrás dela; ao ganhar vida, a barra cresce deslizando.
4. Tudo opcional e regulável no ini; quem não gerar o pacote continua com o xBR atual.

## Não metas

- Desenhar arte nova dos personagens ou trocar o estilo do HUD (os personagens são protegidos; e a escolha foi ser fiel).
- Mudar posição, tamanho ou layout do HUD.
- Os ícones 3D presos à tela (cenoura, moeda de ouro, claquete: modelos 3D posicionados em 0x404d40) só ganham a textura em HD; sombra e animação são só para o HUD 2D.
- 60 fps: é o próximo projeto, separado deste.
- A fonte do jogo (letras da pausa e dos menus): continua no xBR, que já ficou bom.

## Anti-metas (falharia mesmo funcionando)

- Perder detalhe da arte original: pernas, boca e dedos do Taz, orelhas do Pernalonga. O teste rápido com desfoque + limiar mostrou exatamente isso, então esse método está descartado.
- Barra de vida que esconde ou atrasa o dano: o valor real tem de aparecer na hora; a animação é só o rastro.
- Queda de quadros ou travadas ao carregar as fases.
- Qualquer arquivo do jogo no repositório ou nos zips.

## Restrições

- O jogo carrega a OpenGL 1.1 inteira por nome (GetProcAddress); o mod já intercepta parte dela. O caminho de desenho do HUD (glBegin/glVertex ou glDrawArrays) ainda não foi medido.
- As texturas são reconhecidas pelo hash de desenho (`tex_lhash`), que não muda com o "Brilho" do menu Vídeo. Os hashes do HUD já estão em `k_hd_builtin`.
- Só o `bblit2.exe` PT-BR está mapeado. Lógica a 30 Hz.
- O mod lê imagens em BMP (sem biblioteca de PNG); o pacote do HUD usa BMP 32 bits com alfa.

## Abordagens consideradas

| | Abordagem | Prós | Contras |
|---|---|---|---|
| A | Só melhorar o xBR embutido | nada a instalar | teto baixo: 32 px continuam sendo 32 px |
| **B** | **Pacote HD do HUD gerado no PC + efeitos no mod** | qualidade alta, mesma arte, reaproveita o pipeline do pacote dos menus | exige rodar o script uma vez; dois passos para gerar |
| C | Arte nova redesenhada | visual "de hoje" | personagens protegidos; foge do original |

Escolhida: **B** (o Night escolheu "fiel com acabamento moderno").

## Desenho

### 1. Origem das texturas
O mod exporta sozinho as texturas do HUD listadas (cerca de 30, ver lista abaixo) para `bin\hd\hud\fonte\<hash>.bmp` na primeira vez que o jogo as carrega, só quando a pasta `bin\hd` existe. O script lê dali. Assim ninguém precisa do modo debug nem do despejo inteiro.

Texturas do HUD (hash de desenho, tamanho): silhuetas `3ff28889dc2f9625` 128×64 (Taz) e `6153c0570df99ae9` 256×64 (Pernalonga); preenchimentos das barras `16463c6abc682e5c` e `6da516a4789678d7` 128×8; números `8a24a8a3dc058391`…`f07de88f84ea054a` 32×32 (10 texturas); `%` `88d23ca06bc73a6c`; espelho, espelho riscado, X e mira (`81186b738d5b2629`, `c71b14dfca3365ce`, `7052f27c1a3671b8`, `bef4c66e2a96838b`); relógio `4c6bdfe92ed18a2d`; medalhas `2263ff8efc8e2c56`, `48b3a3ee49d533c1`, `3abd3f8592615d20`, `d60f246eed512e05`; emblema `59f8e33e8c711ede`; claquete `76e79fa1ab5f0475`; cenoura `cdfbd6edf13ed09e`; barra laranja `9f99c3526e3e26ae`; setas `1773bbcebaff6d1f`, `814438235e46e7fc`. A lista final sai da medição do passo 1 do plano.

### 2. Geração (script, no PC do jogador)
`tools/hd_menu/make_pack.py` ganha a etapa do HUD (mesmo comando, sem passo extra):
- **Silhuetas** (preto com contorno branco, alfa 0/255): contorno traçado em curvas a partir dos cantos dos pixels, com preservação de cantos e de traços de 1 px, renderizado em 4× com antisserrilhado. As áreas preta e branca são traçadas separadamente para o contorno branco manter a espessura.
- **Ícones coloridos e números**: Real-ESRGAN anime 4× (o mesmo modelo do pacote dos menus), com o alfa ampliado à parte e recortado nítido.
- **Preenchimento das barras** (degradê 128×8): ampliação suave simples; o degradê não precisa de IA.
- Saída: `bin\hd\hud\<hash>.bmp` (32 bits, alfa) e uma folha de comparação `hud_comparacao.png` para conferência.

### 3. Carregamento (mod)
No envio da textura (`hd_replace`), se existir `bin\hd\hud\<hash>.bmp` com 4× o tamanho original, ele é enviado no lugar do xBR. Sem o arquivo, nada muda. Nova opção `[video] hud_hd=1`.

### 4. Sombra suave
Para cada ícone do HUD 2D (textura marcada como HUD, desenhada em projeção 2D), o mod desenha antes uma cópia preta, desfocada e deslocada para baixo e para a direita. A sombra é uma textura gerada pelo próprio mod a partir do alfa da textura HD, com margem para o desfoque caber, e o quadrilátero da sombra é ampliado na mesma proporção. Valores iniciais: deslocamento ~0,15% da altura da tela, desfoque ~0,4%, opacidade 45%. Opção `[hud] shadow=45` (0 desliga).

### 5. Barra de vida animada
O mod reconhece o desenho do preenchimento das barras (textura do degradê) e guarda a largura mostrada de cada barra (jogador da esquerda e da direita).
- Perdeu vida: o preenchimento cai na hora; um rastro mais claro, do tamanho antigo, espera 0,25 s e desce até o novo valor em 0,4 s.
- Ganhou vida: o preenchimento cresce do valor antigo ao novo em 0,3 s.
- Tempo real (QueryPerformanceCounter), não depende do fps. Pausa e troca de fase zeram a animação.
- A forma de "encurtar" segue o que o jogo faz hoje (cortar a textura ou espremer), medido no passo 1.
Opção `[hud] smooth_bars=1`.

### 6. Configuração e documentação
`bblit2fix.ini`: `hud_hd`, `[hud] shadow`, `[hud] smooth_bars`. README (O que muda + tabela de opções), CHANGELOG 0.13, README do `tools/hd_menu`.

## Como verificar

- Funções puras com testes automáticos no build (Linux): animação da barra (tempo → largura), leitura do BMP com alfa, traçado de uma silhueta pequena conhecida.
- No jogo, capturas F12 antes e depois (antes = pacote ausente e efeitos desligados), pausa e meio da fase, conferidas pelo Night.
- Log: quantas texturas do HUD trocadas, tempo do carregamento, fps igual ao de antes.

## Riscos e o que falta medir (primeira tarefa do plano)

1. Por onde o HUD é desenhado (glBegin/glVertex, glDrawArrays ou listas). Isso decide onde interceptar para a sombra e a barra. Medição: build de diagnóstico que conta chamadas por função enquanto uma textura do HUD está ligada.
2. Como a largura da barra muda com a vida (geometria e coordenadas de textura).
3. Se o traçado das silhuetas fica fiel o bastante; se não, comparar com o Real-ESRGAN e escolher por textura, com o Night vendo a folha de comparação.

## Medição (Tarefa 1, 21/11, logs do Night e despejo F10 de um quadro)

- **Caminho do desenho**: o HUD 2D é desenhado em `glOrtho` com `glBegin(GL_TRIANGLES)` e `glVertex3fv`, 2 triângulos (6 vértices) por sprite ou barra. Coordenadas de tela centradas: x de -960 a 960, y de -540 a 540; sprites em z -2, barras em z -1. Não há `glDrawArrays`, `glDrawElements` nem listas.
- **Coordenadas de textura**: não vêm de `glTexCoord2f/2fv`, e sim de outra variante (contada pelo diagnóstico como "TexCoord(outra)"). Todos os sprites e barras usam a textura inteira com uma margem de 0,018 (`0.018..0.982` em s e t).
- **Barras de vida**: a geometria muda, a textura não: o jogo encurta o retângulo (a textura do degradê é espremida). Barra cheia = 296,2 unidades; a do jogador da esquerda começa em x = -701,2 e cresce para a direita, a da direita termina em x = 806,2 e cresce para a esquerda. O jogo **já anima** a barra: 3,75 unidades por quadro, piscando durante a mudança. Por isso a animação do item 5 foi cancelada (ver o registro do plano): um rastro por cima duplicaria o efeito.
- **Posições (tela 16:9)**: silhueta do Pernalonga (256×64) em -915..-394 × 342..612; do Taz (128×64) em 495..926 × 354..624; preenchimento das barras (128×8) com 42 unidades de altura.
- **Atenção para a sombra**: perguntar o estado do GL (`glIsEnabled`) entre `glBegin` e `glEnd` não vale (devolve 0); o ponto de interceptação precisa guardar o estado antes do `glBegin`.

## Decisões deixadas para quem implementar

Algoritmo exato do traçado (potrace ou próprio), parâmetros finos de sombra e animação dentro das faixas acima, organização do código novo em arquivo próprio (`src/hud.c`) ou dentro do `bbfix.c`.

## Decisões minhas (para o Night conferir)

- Rastro mais claro na perda de vida, em vez de a barra inteira descer devagar: mostra o dano na hora e ainda dá o movimento.
- Sombra só no HUD 2D; os ícones 3D ficam só com a textura HD.
- O mod exporta as texturas do HUD automaticamente quando a pasta `bin\hd` existe.
