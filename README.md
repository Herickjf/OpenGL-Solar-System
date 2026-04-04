# OpenGL-Solar-System

Este projeto nasceu como trabalho final da disciplina de Introdução à Computação Gráfica, ministrada pelo Prof. Davi Henrique dos Santos. A proposta foi transformar um conjunto de conceitos de iluminação, câmera, texturas e transformação geométrica em uma cena navegável: um sistema solar interativo, com órbitas, luas, anéis, HUD e foco em corpos celestes.

Mais do que reproduzir planetas, a ideia foi construir uma pequena experiência visual. O Sol atua como emissor principal da cena, os planetas recebem materiais próprios para refletância e brilho, e a Terra usa uma textura noturna para reforçar o contraste entre o lado iluminado e o lado escuro. O resultado é uma simulação que tenta equilibrar fidelidade visual com clareza didática.

## O que o projeto mostra

- Sistema solar com corpos orbitando em tempo real.
- Câmera livre e câmera com foco em planeta ou lua.
- Iluminação geral com materiais por corpo.
- HUD para selecionar rapidamente planetas e luas.
- Texturas, anéis e detalhes visuais por corpo celeste.
- Terra com textura noturna no lado escuro.

## Como compilar e executar

O projeto foi desenvolvido em C com OpenGL/GLUT. Em um ambiente com as bibliotecas instaladas, compile com:

```bash
g++ main.c src/bodies.c src/hud.c libs/cJSON.c src/utils.c src/calculus.c src/input.c src/draw.c src/stb_image.c -Iinclude -o solarSystem -lGL -lGLU -lglut -lGLEW
```

Depois, execute o binário gerado:

```bash
./solarSystem
```

## Controles

- `W`, `A`, `S`, `D` para mover a câmera.
- `Q` e `E` para descer e subir.
- Mouse para olhar ao redor.
- Scroll para aproximar ou afastar.
- `H` para mostrar ou ocultar o HUD.
- `P` para pausar ou retomar a simulação.
- Clique nos botões do HUD para focar um planeta ou lua.

## Estrutura do projeto

- `main.c` inicializa a janela, a cena e o loop principal.
- `src/bodies.c` carrega corpos, texturas e dados do `configs.json`.
- `src/draw.c` cuida do desenho dos planetas, luas, órbitas e fundo.
- `src/hud.c` desenha e gerencia a interface de foco.
- `src/input.c` trata teclado e mouse.
- `src/calculus.c` calcula posições orbitais.
- `configs.json` concentra as configurações visuais e físicas da cena.

## Observação final

Este projeto foi pensado como uma ponte entre teoria e prática: uma simulação simples o bastante para ser entendida, mas rica o bastante para mostrar, na tela, como luz, sombra, textura e movimento trabalham juntos em computação gráfica.
