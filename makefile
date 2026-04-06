# =========================
# Configurações
# =========================
CC = g++
TARGET = solarSystem

SRC = main.c \
	src/bodies.c \
	src/hud.c \
	src/audio_controller.c \
	src/camera_controller.c \
	libs/cJSON.c \
	src/utils.c \
	src/calculus.c \
	src/input.c \
	src/draw.c \
	src/stb_image.c

INCLUDES = -Iinclude

LIBS = -lGL -lGLU -lglut -lGLEW -lSDL2 -lSDL2_mixer

CFLAGS = -g -Wall -O2

# =========================
# Regras principais
# =========================
all: $(TARGET)

$(TARGET):
	$(CC) $(SRC) $(INCLUDES) $(CFLAGS) -o $(TARGET) $(LIBS)

run: all
	./$(TARGET)

clean:
	rm -f $(TARGET)

rebuild: clean all

# =========================
# Instalação de dependências (Ubuntu/WSL)
# =========================
install:
	sudo apt update && sudo apt install -y \
		build-essential \
		libgl1-mesa-dev \
		libglu1-mesa-dev \
		freeglut3-dev \
		libglew-dev \
		libsdl2-dev \
		libsdl2-mixer-dev \
		libmpg123-dev

# =========================
# Debug (opcional)
# =========================
debug:
	$(CC) $(SRC) $(INCLUDES) -g -o $(TARGET) $(LIBS)