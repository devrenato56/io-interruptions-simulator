/*
 * config_sim.h — Parámetros del modelo de E/S dirigida por interrupciones.
 *
 * Este archivo concentra las constantes teóricas del simulador: los
 * dispositivos, sus prioridades (fijas, estilo PIC 8259A), sus números de
 * vector en la IVT y el tiempo de servicio de cada uno.
 *
 * Referencia: Silberschatz, Operating System Concepts (10.ª ed.), Fig. 1.4
 * (Interrupt-driven I/O cycle); Tanenbaum, Structured Computer Organization
 * (PIC 8259A).
 */
#ifndef CONFIG_SIM_H
#define CONFIG_SIM_H

/* Número de procesos de usuario simulados. */
#define N_PROC   3

/* Índices de dispositivo. El Timer es especial (genera el fin de quantum del
 * planificador) y NO se atiende en el lazo de dispositivos de E/S. */
#define DEV_TIMER    0
#define DEV_DISCO    1
#define DEV_TECLADO  2
#define N_DISPOS     3

/* Tablas de configuración por índice de dispositivo (definidas en simulador.c):
 *   DEV_NOMBRE : nombre legible.
 *   DEV_PRIO   : prioridad fija del PIC (MENOR número = MAYOR prioridad).
 *   DEV_VEC    : número de vector en la IVT (0x20..0x22, como IRQ0..IRQ2).
 *   DEV_SERV   : ciclos de servicio (el Timer no se sirve por E/S). */
extern const char *const DEV_NOMBRE[N_DISPOS];
extern const int         DEV_PRIO[N_DISPOS];
extern const int         DEV_VEC[N_DISPOS];
extern const int         DEV_SERV[N_DISPOS];

/* Quantum del planificador (ciclos antes de que el Timer dispare su IRQ). */
#define QUANTUM  4

/* Etapas internas del ISR (1..7). */
#define N_ETAPAS 7

#endif /* CONFIG_SIM_H */
