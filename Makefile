# Makefile — Simulador de interrupciones de E/S (núcleo + GUI en C)
#
#   make        compila la interfaz gráfica (./simulador_gui)  [requiere raylib]
#   make gui    igual que make
#   make cli    compila el simulador de consola/traza (./simulador)
#   make test   compila y ejecuta las pruebas de E/S y CPU Core
#   make run    genera trace.csv con el simulador de consola
#   make clean  elimina binarios y trazas

CC      := gcc
CFLAGS  := -std=c17 -Wall -Wextra -Iinclude -O2
NUCLEO  := src/nucleo/simulador.c src/fuentes/teclado.c
BASE    := src/cpu/cpu_core.c src/contexto/context_switch.c src/ivt/vector_interruptions.c
HEADERS := $(wildcard include/*.h)

EXE :=

ifeq ($(OS),Windows_NT)
  EXE := .exe
  RAYLIBS := -lraylib -lopengl32 -lgdi32 -lwinmm
else
  UNAME := $(shell uname -s)
  ifeq ($(UNAME),Darwin)
    RAYLIBS := -lraylib -framework OpenGL -framework Cocoa -framework IOKit
  else
    RAYLIBS := -lraylib -lGL -lm -lpthread -ldl -lrt -lX11
  endif
endif

BIN     := simulador$(EXE)
GUIBIN  := simulador_gui$(EXE)
TESTBIN := test_interrupciones$(EXE)
CPUTEST := test_cpu$(EXE)

all: gui

gui: $(GUIBIN)

$(GUIBIN): src/ui/main_gui.c $(NUCLEO) $(HEADERS)
	$(CC) $(CFLAGS) -o $@ $(filter %.c,$^) $(RAYLIBS)

cli: $(BIN)

$(BIN): src/nucleo/main_sim.c $(NUCLEO) $(HEADERS)
	$(CC) $(CFLAGS) -o $@ $(filter %.c,$^)

$(TESTBIN): tests/test_interrupciones.c $(NUCLEO) $(HEADERS)
	$(CC) $(CFLAGS) -o $@ $(filter %.c,$^)

$(CPUTEST): tests/test_integration.c $(BASE) $(HEADERS)
	$(CC) $(CFLAGS) -o $@ $(filter %.c,$^)

test: $(TESTBIN) $(CPUTEST)
	./$(TESTBIN)
	./$(CPUTEST)

run: cli
	./$(BIN) --ciclos 400 --salida trace.csv

clean:
	rm -f $(BIN) $(GUIBIN) $(TESTBIN) $(CPUTEST) test_integration.exe tests/test_integration.exe trace.csv

.PHONY: all gui cli test run clean
