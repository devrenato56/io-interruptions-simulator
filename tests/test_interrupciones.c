/*
 * test_interrupciones.c — Verificación de invariantes teóricos del motor.
 *
 * Reproduce las mismas comprobaciones que validaron el simulador web:
 *   1. Una IRQ libera exactamente un proceso (BUG 2).
 *   2. Dos IRQs del mismo dispositivo liberan dos procesos: no se pierden (BUG 1).
 *   3. En una corrida larga ningún proceso queda bloqueado sin dispositivo
 *      ni sufre inanición (BUG 15 + arbitraje justo).
 *   4. Con anidamiento activado ocurren preempciones reales (BUG 9).
 */
#include <stdio.h>
#include "simulador.h"

static int fallos = 0;
static void check(int cond, const char *msg) {
    if (!cond) { printf("  [FALLO] %s\n", msg); fallos++; }
    else       { printf("  [OK]    %s\n", msg); }
}

/* Test 1: una IRQ de Disco con dos procesos en espera libera SOLO uno. */
static void test_una_irq_un_proceso(void) {
    printf("Test 1 — una IRQ libera un solo proceso (BUG 2)\n");
    Simulador S; sim_init(&S);
    S.t_asincrono = 0;
    S.timer = 1000000;                 /* aísla la prueba: el Timer no interfiere */
    S.cpu = -1; S.n_ready = 0;
    /* Tres procesos bloqueados esperando Disco (su E/S ya terminó). */
    for (int i = 0; i < N_PROC; i++) {
        S.proc[i].estado = PROC_BLOQUEADO; S.proc[i].dispositivo = DEV_DISCO; S.proc[i].pendiente = DEV_DISCO;
    }
    /* Una sola IRQ pendiente de Disco. */
    S.pic.irr[DEV_DISCO] = 1; S.pic.irr_count[DEV_DISCO] = 1; S.irq_alzada_en[DEV_DISCO] = 0;
    int antes = S.es, guarda = 0;
    /* Corremos hasta que la IRQ se atienda por completo (ISR terminado). */
    while ((S.pic.irr[DEV_DISCO] || S.in_isr) && guarda < 40) { sim_tick(&S); guarda++; }
    int bloqueados = 0;
    for (int i = 0; i < N_PROC; i++)
        if (S.proc[i].estado == PROC_BLOQUEADO && S.proc[i].pendiente == DEV_DISCO) bloqueados++;
    check(S.es - antes == 1, "1 IRQ → exactamente 1 proceso liberado");
    check(bloqueados == 2, "los otros 2 procesos siguen esperando (no se liberan de más)");
}

/* Test 2: dos IRQs de Disco liberan dos procesos (no se pierde la segunda). */
static void test_dos_irqs_dos_procesos(void) {
    printf("Test 2 — dos IRQs → dos procesos liberados (BUG 1)\n");
    Simulador S; sim_init(&S);
    S.t_asincrono = 0;
    S.timer = 1000000;
    S.cpu = -1; S.n_ready = 0;
    for (int i = 0; i < N_PROC; i++) {
        S.proc[i].estado = PROC_BLOQUEADO; S.proc[i].dispositivo = DEV_DISCO; S.proc[i].pendiente = DEV_DISCO;
    }
    /* Dos IRQs pendientes del mismo dispositivo: no deben perderse. */
    S.pic.irr[DEV_DISCO] = 1; S.pic.irr_count[DEV_DISCO] = 2; S.irq_alzada_en[DEV_DISCO] = 0;
    int antes = S.es, guarda = 0;
    /* Detenemos en cuanto se hayan liberado 2 procesos (evita el reciclaje). */
    while (S.es - antes < 2 && guarda < 80) { sim_tick(&S); guarda++; }
    check(S.es - antes == 2 && guarda < 80,
          "2 IRQs → 2 procesos liberados (la 2.ª IRQ no se pierde)");
}

/* Test 3: corrida larga sin procesos huérfanos ni inanición. */
static void test_corrida_larga(void) {
    printf("Test 3 — corrida larga sin huérfanos ni inanición\n");
    Simulador S; sim_init(&S);
    S.t_anidar = 1;
    int releases[N_PROC] = {0,0,0};
    EstadoProc prev[N_PROC];
    for (int i = 0; i < N_PROC; i++) prev[i] = S.proc[i].estado;
    int ultimo_libre[N_PROC] = {0,0,0};
    int huerfano = 0;
    const int N = 3000;
    for (int t = 0; t < N; t++) {
        sim_tick(&S);
        for (int i = 0; i < N_PROC; i++) {
            EstadoProc e = S.proc[i].estado;
            if (e == PROC_BLOQUEADO && S.proc[i].pendiente < 0 && S.proc[i].dispositivo < 0) huerfano = 1;
            if (prev[i] == PROC_BLOQUEADO && e != PROC_BLOQUEADO) releases[i]++;
            if (e != PROC_BLOQUEADO) ultimo_libre[i] = S.ciclo;
            prev[i] = e;
        }
    }
    check(!huerfano, "ningún proceso queda bloqueado sin dispositivo (integridad)");
    int prog = 1, sinInanicion = 1;
    for (int i = 0; i < N_PROC; i++) {
        if (releases[i] < 3) prog = 0;
        if (S.ciclo - ultimo_libre[i] > 300) sinInanicion = 0;
    }
    printf("        liberaciones por proceso: P1=%d P2=%d P3=%d · E/S=%d\n",
           releases[0], releases[1], releases[2], S.es);
    check(prog, "cada proceso avanza (se libera varias veces)");
    check(sinInanicion, "ningún proceso bloqueado los últimos 300 ciclos (sin inanición)");
}

/* Test 4: el anidamiento produce preempciones reales. */
static void test_anidamiento(void) {
    printf("Test 4 — anidamiento real con STI (BUG 9)\n");
    Simulador S; sim_init(&S);
    S.t_anidar = 1;
    for (int t = 0; t < 3000; t++) sim_tick(&S);
    printf("        anidamientos observados: %d\n", S.nest);
    check(S.nest > 0, "ocurre al menos un anidamiento (antes era imposible)");
}

int main(void) {
    printf("=== Pruebas del motor de interrupciones ===\n\n");
    test_una_irq_un_proceso();
    test_dos_irqs_dos_procesos();
    test_corrida_larga();
    test_anidamiento();
    printf("\n");
    if (fallos == 0) { printf("TODAS LAS PRUEBAS PASARON.\n"); return 0; }
    printf("PRUEBAS FALLIDAS: %d\n", fallos);
    return 1;
}
