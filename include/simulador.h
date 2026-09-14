/*
 * simulador.h — Estado y API del motor de simulación de interrupciones de E/S.
 *
 * El motor reproduce, de forma discreta (por ciclos/"ticks"), el ciclo de E/S
 * dirigida por interrupciones de la Figura 1.4: el driver inicia la E/S, el
 * controlador la ejecuta en paralelo, genera una IRQ, el PIC la arbitra, la
 * CPU la atiende entre instrucciones (guardar contexto → IVT → ISR → EOI →
 * IRET) y reanuda la tarea interrumpida.
 *
 * Es la versión en C, corregida y verificada, del simulador web (v4): incluye
 * PIC con IRR/IMR/ISR y prioridad fija, conteo de IRQs pendientes por
 * dispositivo (para no perder solicitudes), liberación de un proceso por IRQ,
 * anidamiento real de interrupciones con STI, reloj propio de dispositivo
 * (E/S asíncrona) y comprobación de integridad de procesos.
 */
#ifndef SIMULADOR_H
#define SIMULADOR_H

#include "config_sim.h"

/* Estado de un proceso de usuario. */
typedef enum { PROC_LISTO, PROC_EJECUTANDO, PROC_BLOQUEADO } EstadoProc;

/* Un proceso: su avance en el plan, ticks de CPU restantes y ligaduras de E/S. */
typedef struct {
    int        fase;          /* índice actual dentro de su plan */
    int        restante;      /* ciclos de CPU que le faltan a la ráfaga actual */
    EstadoProc estado;
    int        dispositivo;   /* dispositivo al que está ligado mientras hace E/S (-1 = ninguno) */
    int        pendiente;     /* dispositivo cuya E/S terminó y espera su ISR (-1 = ninguno) */
} Proceso;

/* Un dispositivo de E/S con su propio reloj (para el modo asíncrono). */
typedef struct {
    int sirviendo;            /* proceso al que atiende (-1 = libre) */
    int restante;             /* ciclos de servicio que faltan */
    int reloj;                /* reloj propio del dispositivo */
    int cola[N_PROC];         /* cola de procesos en espera */
    int n_cola;
} Dispositivo;

/* Controlador programable de interrupciones (PIC 8259A simplificado). */
typedef struct {
    int irr[N_DISPOS];        /* Interrupt Request Register: línea pendiente (0/1) */
    int imr[N_DISPOS];        /* Interrupt Mask Register: línea enmascarada (0/1) */
    int isr[N_DISPOS];        /* In-Service Register: en servicio (0/1) */
    int irr_count[N_DISPOS];  /* nº de IRQs pendientes por dispositivo (evita perderlas) */
    int pila_isr[16];         /* pila de dispositivos en servicio (para el anidamiento) */
    int n_isr;
    int en_servicio;          /* dispositivo en servicio actual (-1 = ninguno) */
} PIC;

/* Marco de contexto de CPU guardado antes de un ISR. */
typedef struct {
    unsigned pc;
    int      r0;
    unsigned eflags;
    int      ifbit;           /* Interrupt Flag: 1 = IRQ habilitadas */
    int      modo;            /* 0 = usuario, 1 = kernel */
    int      cpu;             /* proceso interrumpido (-1 = ninguno) */
} MarcoContexto;

/* Marco de progreso de un ISR externo preemptado por anidamiento.
 * Guarda TODO el estado de la interrupción externa (incluido si ya emitió su
 * EOI), no solo su etapa, para restaurarlo intacto al reanudarla. */
typedef struct {
    int intr_dev;
    int intr_vec;
    int intr_prio;
    int intr_eoi_enviado;
    int stage;
    int cur_dev;
} MarcoISR;

/* Estado completo del simulador. */
typedef struct {
    int ciclo;

    /* --- CPU --- */
    int      cpu;             /* proceso en CPU (-1 = ninguno) */
    int      modo;            /* 0 = usuario, 1 = kernel */
    unsigned pc;
    int      r0;
    unsigned eflags;
    int      ifbit;

    /* --- Cola de listos --- */
    int ready[N_PROC * 4];
    int n_ready;

    /* --- Procesos y dispositivos --- */
    Proceso     proc[N_PROC];
    Dispositivo dev[N_DISPOS]; /* se usan DEV_DISCO y DEV_TECLADO; Timer es especial */
    int         timer;         /* cuenta regresiva del quantum */
    int         quantum;

    /* --- PIC y ciclo de interrupción --- */
    PIC pic;
    int in_isr;                /* 1 si hay un ISR en curso */
    int stage;                 /* etapa del ISR (1..7) */
    int cur_dev;               /* dispositivo del ISR en curso */
    int intr_dev, intr_vec, intr_prio, intr_eoi_enviado;

    MarcoContexto pila_ctx[16];
    int           n_ctx;
    MarcoISR      pila_nest[8];
    int           n_nest;

    /* --- Métricas --- */
    int irq;                   /* IRQ atendidas */
    int ctx;                   /* cambios de contexto */
    int busy;                  /* ciclos de CPU ocupada */
    int es;                    /* operaciones de E/S completadas */
    int lat_sum;               /* suma de latencias IRQ→ISR */
    int nest;                  /* anidamientos */
    int eoi;                   /* EOI emitidos */
    int irq_alzada_en[N_DISPOS]; /* ciclo en que se alzó la IRQ (-1 = ninguna) */

    /* --- Driver --- */
    int driver_busy;
    int driver_dev;

    /* --- Bitácora en memoria (más reciente primero), para la interfaz --- */
    char logtxt[SIM_LOG_MAX][100];
    int  logtag[SIM_LOG_MAX];   /* 0 = SYS, 1 = IRQ, 2 = DRV */
    int  logt[SIM_LOG_MAX];     /* ciclo del evento */
    int  nlog;

    /* --- Toggles / variantes didácticas --- */
    int t_anidar;              /* anidamiento con STI dentro del ISR */
    int t_eoi_temprano;        /* EOI antes de procesar el ISR */
    int t_asincrono;           /* dispositivo con reloj propio (E/S asíncrona) */
} Simulador;

/* Inicializa el simulador a su estado de arranque. */
void sim_init(Simulador *S);

/* Avanza un ciclo (un "tick") de la simulación. */
void sim_tick(Simulador *S);

/* Devuelve la máscara de 3 bits (Timer,Disco,Teclado) de un registro del PIC. */
void sim_mascara(const int reg[N_DISPOS], char out[N_DISPOS + 1]);

#endif /* SIMULADOR_H */
