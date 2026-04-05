# Sistema Solar em OpenGL

**Trabalho final — Introdução à Computação Gráfica**  
Disciplina ministrada pelo **Prof. Davi Henrique dos Santos**.

## Resumo

Este projeto implementa uma cena tridimensional navegável de um sistema solar estilizado em **C**, utilizando **OpenGL** (com **GLUT** para janela e entrada, **GLEW** para extensões e **GLU** para utilitários). A aplicação integra iluminação por material, mapeamento de texturas (incluindo textura secundária para o “lado noturno” da Terra), órbitas elípticas simplificadas, anéis planetários, fundo estelar e uma interface na tela (HUD) para foco rápido em planetas e luas. O objetivo é demonstrar, de forma didática, como conceitos de **transformação geométrica**, **câmera**, **iluminação** e **textura** se combinam numa simulação em tempo real.

---

## 1. Objetivos

- Aplicar o pipeline gráfico fixo do OpenGL (matrizes de modelo/visualização/projeção, iluminação `GL_LIGHTING`).
- Modelar corpos celestes com dados externos (**JSON**), permitindo ajuste de escalas, períodos e aparência sem recompilar.
- Oferecer **interação**: câmera livre, seguimento de corpo (planeta ou lua), pausa e alternância do HUD.
- Reforçar leitura visual: Sol como fonte de luz aparente (emissão), contrastes de materiais e, na Terra, sobreposição noturna orientada à direção da luz.

---

## 2. Conceitos de computação gráfica utilizados

| Tema | Uso no projeto |
|------|----------------|
| **Transformações** | Translação e rotação dos corpos, inclinação orbital, rotação axial e hierarquia planeta–lua. |
| **Câmera** | Modo livre (WASD, mouse, scroll) e modo “seguir” com interpolação suave até o alvo selecionado. |
| **Iluminação** | Modelo de Phong via `glMaterial` (ambiente derivado do difuso, difuso, especular, brilho e emissão). |
| **Texturas** | Mapeamento esférico; texturas normais onde configuradas; textura secundária para noite na Terra e atmosfera no Vênus; anéis com textura dedicada. |
| **Cena e tempo** | Loop de animação com controle de taxa de quadros; tempo de simulação acumulado para ângulos orbitais e de rotação. |

---

## 3. Modelo orbital e escalas

As posições orbitais são calculadas em `src/calculus.c` a partir de uma **elipse** em plano XZ, com **excentricidade** e **inclinação** lidas do JSON. O raio instantâneo segue a forma usual da elipse em coordenadas polares:

\[
r = \frac{a\,(1 - e^2)}{1 + e\cos\theta}
\]

em que \(a\) é o semi-eixo (escalado por `distance_scale`) e \(e\) a excentricidade. **Não** se trata de um integrador N-corpos: as órbitas são **keplerianas simplificadas** para efeito visual e pedagógico.

No arquivo `configs.json`, a seção `scale` define `distance_scale`, `radius_scale` e `time_scale`, permitindo comprimir distâncias e acelerar o tempo sem alterar o código.

---

## 4. Arquitetura do software

| Módulo | Função |
|--------|--------|
| `main.c` | Inicialização (GLEW/GLUT), estado global da câmera e simulação, temporizador, atualização de “seguir” alvo e loop de desenho. |
| `src/bodies.c` | Leitura de `configs.json` com **cJSON**, carregamento de texturas (**stb_image**), montagem da lista de corpos, luas e anéis. |
| `src/calculus.c` | Posição de planetas e luas em função de `time_sim`. |
| `src/draw.c` | Desenho do fundo, esferas, órbitas, anéis e sobreposição noturna da Terra (malha esférica com mistura por orientação à luz). |
| `src/input.c` | Teclado, mouse e scroll. |
| `src/hud.c` | Painel de seleção e foco em corpos. |
| `src/utils.c` | Utilitários compartilhados. |
| `include/*.h` | Tipos (`Body`, `Moon`, `Rings`, `Camera`, `Material`, etc.) e declarações. |

**Dados da cena:** `configs.json` descreve o Sol, Mercúrio, Vênus, Terra (e Lua), Marte (Fobos e Deimos), Júpiter, Saturno (anéis), Urano, Netuno e Plutão, além da textura do campo de estrelas e parâmetros globais de iluminação.

**Bibliotecas de terceiros (incluídas no repositório):**

- [cJSON](https://github.com/DaveGamble/cJSON) — parsing JSON.
- [stb_image](https://github.com/nothings/stb) — carregamento de imagens para texturas.

---

## 5. Requisitos

- Compilador **C/C++** (`g++` ou compatível).
- **OpenGL**, **GLU**, **GLUT** (ou FreeGLUT) e **GLEW** instalados e visíveis ao linker.
- Execução a partir do diretório onde estão `configs.json` e a pasta `textures/` (caminhos relativos no JSON).

---

## 6. Compilação e execução

### Linux (exemplo típico)

```bash
g++ main.c src/bodies.c src/hud.c libs/cJSON.c src/utils.c src/calculus.c src/input.c src/draw.c src/stb_image.c -Iinclude -o solarSystem -lGL -lGLU -lglut -lGLEW
g++ main.c src/bodies.c src/hud.c src/audio_controller.c src/camera_controller.c libs/cJSON.c src/utils.c src/calculus.c src/input.c src/draw.c src/stb_image.c -Iinclude -o solarSystem -lGL -lGLU -lglut -lGLEW -lSDL2 -lSDL2_mixer && ./solarSystem 

```

Depois, execute o binário gerado:

```bash
./solarSystem
```

### Windows

Em ambientes tipo **MSYS2 / MinGW**, os nomes das bibliotecas podem diferir (por exemplo `-lopengl32 -lglu32 -lfreeglut -lglew32`). Ajuste os flags conforme o seu kit de desenvolvimento; a lista de arquivos-fonte permanece a mesma.
- `W`, `A`, `S`, `D` para mover a câmera.
- `Q` e `E` para descer e subir.
- Mouse para olhar ao redor.
- Scroll para aproximar ou afastar.
- `H` para mostrar ou ocultar o HUD.
- `P` para pausar ou retomar a simulação.
- Clique nos botões do HUD para focar um planeta ou lua.
- `M` para pausar/retomar a reprodução de músicas.

> **Importante:** execute o binário na **raiz do projeto** (onde está `configs.json`), para que os caminhos `./textures/...` sejam resolvidos corretamente.

---

## 7. Controles

| Entrada | Ação |
|---------|------|
| `W` `A` `S` `D` | Deslocar a câmera no plano horizontal. |
| `Q` / `E` | Descer / subir. |
| Mouse | Orientar a visão. |
| Scroll | Aproximar ou afastar (zoom). |
| `H` | Mostrar ou ocultar o HUD. |
| `P` | Pausar ou retomar a simulação. |
| Clique no HUD | Focar planeta ou lua indicado. |

---

## 8. Estrutura de diretórios (visão geral)

```text
OpenGL-Solar-System/
├── main.c
├── configs.json
├── include/          # cabeçalhos (.h)
├── src/              # implementação (.c)
├── libs/             # cJSON e stb_image
└── textures/         # assets referenciados pelo JSON
```

---

## 9. Limitações e extensões possíveis

- O modelo é **visual e didático**, não astronômico: massas, perturbações e inclinações reais estão simplificadas ou ausentes.
- Iluminação e sombras seguem o modelo clássico do OpenGL imediato; não há sombras projetadas nem *deferred shading*.
- Como evolução natural do trabalho: shader programs modernos (GLSL), *skybox* explícito, ou leitura de efemérides mais realistas.

---

## 10. Conclusão

O projeto funciona como uma **ponte entre teoria e prática**: o código permanece legível o suficiente para localizar cada ideia de computação gráfica (transformação, material, textura, câmera, tempo), enquanto a cena final oferece uma experiência coerente de **sistema solar interativo** adequada a um trabalho de Introdução à Computação Gráfica.
