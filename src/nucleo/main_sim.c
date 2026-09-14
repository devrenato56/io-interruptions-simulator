/*
 * main_sim.c — Punto de entrada del simulador de interrupciones de E/S.
 *
 * Ejecuta N ciclos del motor y, opcionalmente, vuelca una traza CSV (una fila
 * por ciclo) que puede animarse después con la herramienta de visualización
 * (viz/animacion.py) o cualquier graficador.
 *
 * Uso:
 *   ./simulador [opciones]
 *     --ciclos N       número de ciclos a simular (por defecto 400)
 *     --anidar         habilita el anidamiento de interrupciones (STI)
 *     --eoi-temprano   envía el EOI antes de procesar el ISR
 *     --sincrono       dispositivos sincronizados con la CPU (sin reloj propio)
 *     --salida ARCH    archivo CSV de traza (por defecto trace.csv; "-" = ninguno)
 *     --verboso        imprime la bitácora de eventos por consola
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "simulador.h"

extern int sim_verboso;

static const char *estado_char(EstadoProc e) {
    switch (e) { case PROC_LISTO: return "L"; case PROC_EJECUTANDO: return "E"; default: return "B"; }
}

static void escribir_cabecera(FILE *f) {
    fprintf(f, "ciclo,cpu,modo,pc,ifbit,n_ready,irr,imr,isr,en_servicio,in_isr,stage,"
               "cur_dev,vec,timer,driver_busy,disco_serv,disco_rem,teclado_serv,teclado_rem,"
               "p1,p2,p3,irq,ctx,es,nest,eoi,pend\n");
}

static void escribir_fila(FILE *f, const Simulador *S) {
    char irr[N_DISPOS + 1], imr[N_DISPOS + 1], isr[N_DISPOS + 1];
    sim_mascara(S->pic.irr, irr);
    sim_mascara(S->pic.imr, imr);
    sim_mascara(S->pic.isr, isr);
    int pend = 0;
    for (int i = 0; i < N_PROC; i++) if (S->proc[i].pendiente >= 0) pend++;

    const Dispositivo *disco = &S->dev[DEV_DISCO];
    const Dispositivo *tecl  = &S->dev[DEV_TECLADO];

    fprintf(f, "%d,%d,%s,0x%X,%d,%d,%s,%s,%s,%s,%d,%d,%s,0x%X,%d,%d,%d,%d,%d,%d,%s,%s,%s,%d,%d,%d,%d,%d,%d\n",
        S->ciclo,
        S->cpu < 0 ? 0 : S->cpu + 1,
        S->modo ? "kernel" : "usuario",
        S->pc, S->ifbit, S->n_ready,
        irr, imr, isr,
        S->pic.en_servicio < 0 ? "-" : DEV_NOMBRE[S->pic.en_servicio],
        S->in_isr, S->stage,
        S->cur_dev < 0 ? "-" : DEV_NOMBRE[S->cur_dev],
        S->in_isr ? S->intr_vec : 0,
        S->timer, S->driver_busy,
        disco->sirviendo < 0 ? 0 : disco->sirviendo + 1, disco->restante < 0 ? 0 : disco->restante,
        tecl->sirviendo  < 0 ? 0 : tecl->sirviendo  + 1, tecl->restante  < 0 ? 0 : tecl->restante,
        estado_char(S->proc[0].estado), estado_char(S->proc[1].estado), estado_char(S->proc[2].estado),
        S->irq, S->ctx, S->es, S->nest, S->eoi, pend);
}

int main(int argc, char **argv) {
    int ciclos = 400;
    const char *salida = "trace.csv";
    Simulador S;
    sim_init(&S);

    for (int i = 1; i < argc; i++) {
        if      (!strcmp(argv[i], "--anidar"))       S.t_anidar = 1;
        else if (!strcmp(argv[i], "--eoi-temprano"))  S.t_eoi_temprano = 1;
        else if (!strcmp(argv[i], "--sincrono"))      S.t_asincrono = 0;
        else if (!strcmp(argv[i], "--verboso"))       sim_verboso = 1;
        else if (!strcmp(argv[i], "--ciclos") && i + 1 < argc) ciclos = atoi(argv[++i]);
        else if (!strcmp(argv[i], "--salida") && i + 1 < argc) salida = argv[++i];
        else { fprintf(stderr, "Opción desconocida: %s\n", argv[i]); return 2; }
    }

    FILE *f = NULL;
    if (strcmp(salida, "-") != 0) {
        f = fopen(salida, "w");
        if (!f) { perror("No se pudo abrir el archivo de traza"); return 1; }
        escribir_cabecera(f);
    }

    printf("Simulador de interrupciones de E/S — %d ciclos%s%s%s\n", ciclos,
           S.t_anidar ? " · anidar" : "",
           S.t_eoi_temprano ? " · EOI-temprano" : "",
           S.t_asincrono ? " · asíncrono" : " · síncrono");

    for (int t = 0; t < ciclos; t++) {
        sim_tick(&S);
        if (f) escribir_fila(f, &S);
    }
    if (f) fclose(f);

    printf("\n== Resumen ==\n");
    printf("  ciclos           : %d\n", S.ciclo);
    printf("  IRQ atendidas    : %d\n", S.irq);
    printf("  E/S completadas  : %d\n", S.es);
    printf("  cambios de ctx   : %d\n", S.ctx);
    printf("  anidamientos     : %d\n", S.nest);
    printf("  EOI emitidos     : %d\n", S.eoi);
    printf("  uso de CPU       : %d%%\n", S.ciclo ? (int)(100L * S.busy / S.ciclo) : 0);
    printf("  latencia IRQ→ISR : %d ciclos (prom.)\n", S.irq ? S.lat_sum / S.irq : 0);
    if (strcmp(salida, "-") != 0) printf("  traza            : %s\n", salida);
    return 0;
}
