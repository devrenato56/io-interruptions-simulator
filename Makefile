# Makefile — Simulador de interrupciones de E/S (núcleo en C)
#
#   make            compila el simulador (binario ./simulador)
#   make test       compila y ejecuta las pruebas de invariantes
#   make run        ejecuta una simulación de ejemplo y genera trace.csv
#   make clean      elimina binarios y trazas

CC      := gcc
CFLAGS  := -std=c17 -Wall -Wextra -Iinclude -O2
NUCLEO  := src/nucleo/simulador.c

BIN     := simulador
TESTBIN := test_interrupciones

all: $(BIN)

$(BIN): src/nucleo/main_sim.c $(NUCLEO)
	$(CC) $(CFLAGS) -o $@ $^

$(TESTBIN): tests/test_interrupciones.c $(NUCLEO)
	$(CC) $(CFLAGS) -o $@ $^

test: $(TESTBIN)
	./$(TESTBIN)

run: $(BIN)
	./$(BIN) --ciclos 400 --salida trace.csv

clean:
	rm -f $(BIN) $(TESTBIN) trace.csv

.PHONY: all test run clean
