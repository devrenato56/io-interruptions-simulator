# Makefile — Simulador de interrupciones de E/S (núcleo + GUI en C)
#
#   make        compila la interfaz gráfica (./simulador_gui)  [requiere raylib]
#   make gui    igual que make
#   make cli    compila el simulador de consola/traza (./simulador)
#   make test   compila y ejecuta las pruebas de invariantes
#   make run    genera trace.csv con el simulador de consola
#   make clean  elimina binarios y trazas

CC      := gcc
CFLAGS  := -std=c17 -Wall -Wextra -Iinclude -O2
NUCLEO  := src/nucleo/simulador.c

BIN     := simulador
GUIBIN  := simulador_gui
TESTBIN := test_interrupciones

ifeq ($(OS),Windows_NT)
  RAYLIBS := -lraylib -lopengl32 -lgdi32 -lwinmm
else
  UNAME := $(shell uname -s)
  ifeq ($(UNAME),Darwin)
    RAYLIBS := -lraylib -framework OpenGL -framework Cocoa -framework IOKit
  else
    RAYLIBS := -lraylib -lGL -lm -lpthread -ldl -lrt -lX11
  endif
endif

all: gui

gui: src/ui/main_gui.c $(NUCLEO)
	$(CC) $(CFLAGS) -o $(GUIBIN) $^ $(RAYLIBS)

cli: src/nucleo/main_sim.c $(NUCLEO)
	$(CC) $(CFLAGS) -o $(BIN) $^

$(TESTBIN): tests/test_interrupciones.c $(NUCLEO)
	$(CC) $(CFLAGS) -o $@ $^

test: $(TESTBIN)
	./$(TESTBIN)

run: cli
	./$(BIN) --ciclos 400 --salida trace.csv

clean:
	rm -f $(BIN) $(GUIBIN) $(TESTBIN) trace.csv

.PHONY: all gui cli test run clean
