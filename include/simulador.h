/* Un flujo de CPU, dispositivos de E/S y sus interrupciones. */
#ifndef SIMULADOR_H
#define SIMULADOR_H
#include "config_sim.h"

typedef struct {
    int id;
    int ciclo_irq;
} FinalizacionES;

typedef struct {
    int sirviendo;             /* ID de solicitud, -1 si esta libre */
    int restante;
    int reloj;                 /* tiempo de servicio, no genera IRQ por si solo */
    int cola[MAX_SOLICITUDES];  /* solicitudes pendientes de transferencia */
    int n_cola;
    FinalizacionES pendientes[MAX_SOLICITUDES]; /* transferidas, esperan ISR */
    int n_pendientes;
    int completadas;           /* solicitudes atendidas por la ISR */
    int ultima_atendida;
} Dispositivo;

/* PIC didactico: IRR/IMR/ISR y conteo de eventos por linea. */
typedef struct {
    int irr[N_DISPOS], imr[N_DISPOS], isr[N_DISPOS];
    int irr_count[N_DISPOS];
    int pila_isr[N_DISPOS];
    int n_isr;
    int en_servicio;
} PIC;

typedef struct {
    unsigned pc;
    int r0;
    unsigned eflags;
    int ifbit;
    int modo;
} MarcoContexto;

/* Progreso de la ISR interrumpida, incluido su reconocimiento al PIC. */
typedef struct {
    int intr_dev, intr_vec, intr_prio, intr_eoi_enviado;
    int stage, cur_dev;
} MarcoISR;

typedef struct {
    int ciclo;
    int modo;
    unsigned pc;
    int r0;
    unsigned eflags;
    int ifbit;

    Dispositivo dev[N_DISPOS];
    int siguiente_solicitud;
    int demo_instrucciones;    /* avance del unico flujo que solicita E/S */
    int demo_dispositivo;

    PIC pic;
    int in_isr, stage, cur_dev;
    int intr_dev, intr_vec, intr_prio, intr_eoi_enviado;
    MarcoContexto pila_ctx[N_DISPOS];
    int n_ctx;
    MarcoISR pila_nest[N_DISPOS];
    int n_nest;

    int irq;
    int contextos_guardados;   /* entradas a ISR, no cambios de tarea */
    int busy;                 /* instrucciones del flujo interrumpible */
    int es;
    int lat_sum;
    int nest;
    int eoi;
    int irq_alzada_en[N_DISPOS];

    int driver_busy, driver_dev;
    char logtxt[SIM_LOG_MAX][100];
    int logtag[SIM_LOG_MAX], logt[SIM_LOG_MAX], nlog;

    int t_anidar;
    int t_eoi_temprano;
    int t_asincrono;
    int t_demo;                /* peticiones automaticas del flujo de CPU */
} Simulador;

void sim_init(Simulador *S);
void sim_tick(Simulador *S);
/* Devuelve ID positivo, o -1 si el dispositivo es invalido o esta lleno.
 * Rechazar una solicitud no modifica el estado ni genera una IRQ. */
int sim_solicitar_es(Simulador *S, int dispositivo);
/* Orden de bits: Disco, Teclado. */
void sim_mascara(const int reg[N_DISPOS], char out[N_DISPOS + 1]);

#endif
