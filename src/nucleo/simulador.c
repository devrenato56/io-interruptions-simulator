/*
 * simulador.c — Motor del ciclo de E/S dirigida por interrupciones (Fig. 1.4).
 *
 * Port en C, corregido y verificado, del simulador web v4. Cada llamada a
 * sim_tick() avanza un ciclo discreto. La lógica reproduce fielmente el ciclo:
 *
 *   1-2  El device driver inicia la E/S (traduce la petición a registros del
 *        controlador) y la CPU se la entrega al I/O controller.
 *   3    El controlador ejecuta la E/S en paralelo (reloj propio del
 *        dispositivo si la E/S es asíncrona).
 *   4    Al terminar, el controlador genera la IRQ; el PIC marca el bit en IRR.
 *   5    Entre instrucciones, la CPU detecta la IRQ: INTA, guarda contexto,
 *        consulta la IVT y salta al handler.
 *   6    El handler procesa, emite EOI y retorna (IRET).
 *   7    La CPU reanuda la tarea interrumpida.
 *
 * Correcciones incorporadas desde el inicio (bugs detectados en el web):
 *   - Conteo de IRQs por dispositivo (no se pierden solicitudes repetidas).
 *   - Una IRQ libera EXACTAMENTE un proceso del dispositivo.
 *   - El arbitraje espera ≥1 ciclo tras alzar la IRQ (maduración).
 *   - Reloj propio del dispositivo para la E/S asíncrona.
 *   - Guardado/restauración de modo (usuario/kernel) en la pila de contexto.
 *   - Anidamiento REAL: con STI y toggle "anidar", una IRQ de mayor prioridad
 *     preempta al ISR en curso; el externo se reanuda al terminar el interno.
 *   - Comprobación de integridad: ningún proceso queda bloqueado sin causa.
 */
#include <stdio.h>
#include <string.h>

#include "simulador.h"

/* Bandera de traza por consola (0 = silencioso). */
int sim_verboso = 0;

/* Tablas de configuración de dispositivos (declaradas en config_sim.h). */
const char *const DEV_NOMBRE[N_DISPOS] = { "Timer", "Disco", "Teclado" };
const int         DEV_PRIO[N_DISPOS]   = { 0, 1, 2 };
const int         DEV_VEC[N_DISPOS]    = { 0x20, 0x21, 0x22 };
const int         DEV_SERV[N_DISPOS]   = { 0, 6, 4 };

/* Plan de cada proceso: alterna ráfagas de CPU y operaciones de E/S.
 * tipo 0 = CPU (arg = ciclos), tipo 1 = E/S (arg = índice de dispositivo). */
typedef struct { int tipo; int arg; } Paso;
#define N_PASOS 4
static const Paso PLAN[N_PROC][N_PASOS] = {
    { {0,3}, {1,DEV_DISCO},   {0,2}, {1,DEV_TECLADO} },
    { {0,2}, {1,DEV_TECLADO}, {0,3}, {1,DEV_DISCO}   },
    { {0,2}, {1,DEV_DISCO},   {0,2}, {1,DEV_TECLADO} },
};

/* --------------------------- utilidades --------------------------- */

static void logmsg(Simulador *S, const char *etq, const char *msg) {
    if (sim_verboso) printf("  t%-4d [%s] %s\n", S->ciclo, etq, msg);
    /* Guarda el evento en la bitácora en memoria (más reciente primero). */
    int tag = (etq[0] == 'I') ? 1 : (etq[0] == 'D') ? 2 : 0;  /* IRQ / DRV / SYS */
    for (int i = SIM_LOG_MAX - 1; i > 0; i--) {
        memcpy(S->logtxt[i], S->logtxt[i - 1], sizeof S->logtxt[0]);
        S->logtag[i] = S->logtag[i - 1];
        S->logt[i]   = S->logt[i - 1];
    }
    snprintf(S->logtxt[0], sizeof S->logtxt[0], "%s", msg);
    S->logtag[0] = tag;
    S->logt[0]   = S->ciclo;
    if (S->nlog < SIM_LOG_MAX) S->nlog++;
}

static void ready_push(Simulador *S, int id) {
    if (S->n_ready < (int)(sizeof S->ready / sizeof S->ready[0]))
        S->ready[S->n_ready++] = id;
}

static int ready_shift(Simulador *S) {
    if (S->n_ready == 0) return -1;
    int v = S->ready[0];
    for (int i = 1; i < S->n_ready; i++) S->ready[i - 1] = S->ready[i];
    S->n_ready--;
    return v;
}

void sim_mascara(const int reg[N_DISPOS], char out[N_DISPOS + 1]) {
    /* Orden de bits: Timer, Disco, Teclado. */
    static const int orden[N_DISPOS] = { DEV_TIMER, DEV_DISCO, DEV_TECLADO };
    for (int i = 0; i < N_DISPOS; i++) out[i] = reg[orden[i]] ? '1' : '0';
    out[N_DISPOS] = '\0';
}

/* --------------------------- PIC --------------------------- */

/* Alza una IRQ del dispositivo d. Cuenta las solicitudes para no perder las que
 * lleguen mientras el bit ya está en el IRR (corrección BUG 1). */
static void pic_alzar(Simulador *S, int d) {
    S->pic.irr_count[d]++;
    S->pic.irr[d] = 1;
    if (S->irq_alzada_en[d] < 0) S->irq_alzada_en[d] = S->ciclo; /* latencia: 1ª IRQ */
}

/* Selecciona el dispositivo a atender: no enmascarado, maduro (≥1 ciclo desde
 * que se alzó) y de mayor prioridad. Con un ISR en curso solo anida si el
 * toggle "anidar" está activo y la nueva IRQ es de prioridad estrictamente
 * mayor. Devuelve el índice de dispositivo o -1. */
static int pic_arbitrar(const Simulador *S) {
    int mejor = -1;
    for (int d = 0; d < N_DISPOS; d++) {
        if (!S->pic.irr[d] || S->pic.imr[d]) continue;
        int alzada = S->irq_alzada_en[d] < 0 ? S->ciclo : S->irq_alzada_en[d];
        if (S->ciclo - alzada < 1) continue;               /* BUG 6: maduración */
        if (S->pic.en_servicio >= 0) {
            if (!S->t_anidar) continue;
            if (!(DEV_PRIO[d] < DEV_PRIO[S->pic.en_servicio])) continue;
        }
        if (mejor < 0 || DEV_PRIO[d] < DEV_PRIO[mejor]) mejor = d;
    }
    return mejor;
}

/* Descuenta una IRQ pendiente del dispositivo; limpia el bit del IRR solo
 * cuando ya no quedan solicitudes de ese dispositivo (corrección BUG 1). */
static void pic_consumir(Simulador *S, int d) {
    if (S->pic.irr_count[d] > 0) S->pic.irr_count[d]--;
    if (S->pic.irr_count[d] <= 0) {
        S->pic.irr_count[d] = 0;
        S->pic.irr[d] = 0;
        S->irq_alzada_en[d] = -1;
    } else {
        S->irq_alzada_en[d] = S->ciclo; /* quedan IRQs: reinicia la marca */
    }
}

static void pic_eoi(Simulador *S, int d) {
    S->pic.isr[d] = 0;
    /* quitar d de la pila de servicio */
    int w = 0;
    for (int i = 0; i < S->pic.n_isr; i++)
        if (S->pic.pila_isr[i] != d) S->pic.pila_isr[w++] = S->pic.pila_isr[i];
    S->pic.n_isr = w;
    S->pic.en_servicio = S->pic.n_isr ? S->pic.pila_isr[S->pic.n_isr - 1] : -1;
    S->eoi++;
}

/* --------------------------- contexto --------------------------- */

static void ctx_guardar(Simulador *S) {
    MarcoContexto c = { S->pc, S->r0, S->eflags, S->ifbit, S->modo, S->cpu };
    S->pila_ctx[S->n_ctx++] = c;
    S->modo = 1; /* entra a kernel */
}

static void ctx_restaurar(Simulador *S) {
    if (S->n_ctx <= 0) return;
    MarcoContexto c = S->pila_ctx[--S->n_ctx];
    S->pc = c.pc; S->r0 = c.r0; S->eflags = c.eflags; S->ifbit = c.ifbit; S->modo = c.modo;
}

/* --------------------------- efectos del ISR --------------------------- */

/* Aplica el efecto de la interrupción atendida sobre los procesos. */
static void aplicar_efecto(Simulador *S) {
    int dev = S->intr_dev;
    if (dev == DEV_TIMER) {
        /* Fin de quantum: expulsa al proceso en CPU. */
        if (S->cpu >= 0) {
            S->proc[S->cpu].estado = PROC_LISTO;
            ready_push(S, S->cpu);
            S->ctx++;
            S->cpu = -1;
        }
    } else {
        /* Una IRQ = una transferencia completa = UN proceso liberado (BUG 2). */
        for (int id = 0; id < N_PROC; id++) {
            if (S->proc[id].pendiente == dev) {
                S->proc[id].pendiente   = -1;
                S->proc[id].estado      = PROC_LISTO;
                S->proc[id].dispositivo = -1;
                ready_push(S, id);
                S->es++;
                break;
            }
        }
    }
}

/* --------------------------- inicialización --------------------------- */

void sim_init(Simulador *S) {
    memset(S, 0, sizeof *S);
    S->ciclo   = 0;
    S->cpu     = -1;
    S->modo    = 0;      /* usuario */
    S->pc      = 0x0400;
    S->r0      = 0;
    S->eflags  = 0x0202;
    S->ifbit   = 1;
    S->timer   = QUANTUM;
    S->quantum = QUANTUM;

    for (int i = 0; i < N_PROC; i++) {
        S->proc[i].fase        = 0;
        S->proc[i].restante    = 0;
        S->proc[i].estado      = PROC_LISTO;
        S->proc[i].dispositivo = -1;
        S->proc[i].pendiente   = -1;
        ready_push(S, i);
    }
    for (int d = 0; d < N_DISPOS; d++) {
        S->dev[d].sirviendo = -1;
        S->irq_alzada_en[d] = -1;
    }
    S->pic.en_servicio = -1;
    S->cur_dev = -1;
    S->intr_dev = -1;
    S->driver_dev = -1;

    /* Toggles por defecto: E/S asíncrona activa (como el web). */
    S->t_anidar = 0;
    S->t_eoi_temprano = 0;
    S->t_asincrono = 1;
}

/* --------------------------- ciclo (tick) --------------------------- */

void sim_tick(Simulador *S) {
    char buf[160];
    S->ciclo++;

    /* ============ Hay un ISR en curso ============ */
    if (S->in_isr) {
        /* --- Anidamiento real (BUG 9): con STI (ifbit=1) y "anidar", una IRQ
         *     de mayor prioridad preempta el ISR en curso. --- */
        if (S->t_anidar && S->ifbit == 1 && S->pic.en_servicio >= 0) {
            int nd = -1;
            for (int d = 0; d < N_DISPOS; d++) {
                if (d == DEV_TIMER) continue;
                if (!S->pic.irr[d] || S->pic.imr[d]) continue;
                if (!(DEV_PRIO[d] < DEV_PRIO[S->pic.en_servicio])) continue;
                int alzada = S->irq_alzada_en[d] < 0 ? S->ciclo : S->irq_alzada_en[d];
                if (S->ciclo - alzada < 1) continue;
                if (nd < 0 || DEV_PRIO[d] < DEV_PRIO[nd]) nd = d;
            }
            if (nd >= 0) {
                int alzada = S->irq_alzada_en[nd] < 0 ? S->ciclo : S->irq_alzada_en[nd];
                MarcoISR m = { S->intr_dev, S->intr_vec, S->intr_prio,
                               S->intr_eoi_enviado, S->stage, S->cur_dev };
                S->pila_nest[S->n_nest++] = m;
                pic_consumir(S, nd);
                S->pic.isr[nd] = 1;
                S->pic.pila_isr[S->pic.n_isr++] = nd;
                S->pic.en_servicio = nd;
                S->intr_dev = nd; S->intr_vec = DEV_VEC[nd]; S->intr_prio = DEV_PRIO[nd];
                S->intr_eoi_enviado = 0;
                S->cur_dev = nd; S->stage = 1; S->irq++; S->nest++;
                S->ifbit = 0;
                S->lat_sum += (S->ciclo - alzada) > 0 ? (S->ciclo - alzada) : 1;
                snprintf(buf, sizeof buf, "[ANIDADA] IRQ de %s (prio %d) preempta el ISR de %s.",
                         DEV_NOMBRE[nd], DEV_PRIO[nd], DEV_NOMBRE[m.cur_dev]);
                logmsg(S, "IRQ", buf);
                return;
            }
        }

        if (S->stage < N_ETAPAS) {
            S->stage++;
            int st = S->stage;
            if (st == 2) { logmsg(S, "SYS", "Paso 2 — INTA (IACK) hacia el PIC."); }
            if (st == 3) { ctx_guardar(S); logmsg(S, "SYS", "Paso 5 — contexto guardado (EFLAGS, CS, EIP)."); }
            if (st == 4) {
                snprintf(buf, sizeof buf, "Paso 5 — IVT consultada, vector 0x%02X → isr_%s().",
                         S->intr_vec, DEV_NOMBRE[S->intr_dev]);
                logmsg(S, "SYS", buf);
                if (S->t_anidar) { S->ifbit = 1; logmsg(S, "SYS", "ISR hace STI → anidamiento permitido (IF=1)."); }
            }
            if (st == 5) {
                if (S->t_eoi_temprano && !S->intr_eoi_enviado) {
                    S->intr_eoi_enviado = 1; pic_eoi(S, S->intr_dev);
                    logmsg(S, "SYS", "Paso 6 — EOI temprano enviado al PIC.");
                }
            }
            if (st == 6) {
                aplicar_efecto(S);
                if (!S->intr_eoi_enviado) {
                    S->intr_eoi_enviado = 1; pic_eoi(S, S->intr_dev);
                    logmsg(S, "SYS", "Paso 6 — EOI enviado al PIC.");
                }
            }
            if (st == 7) { ctx_restaurar(S); logmsg(S, "SYS", "Paso 7 — contexto restaurado (IRET)."); }
            return;
        } else {
            /* ISR terminado: si preemptó a otro, reanuda el externo. */
            if (S->n_nest > 0) {
                MarcoISR m = S->pila_nest[--S->n_nest];
                S->intr_dev = m.intr_dev; S->stage = m.stage; S->cur_dev = m.cur_dev;
                S->intr_vec = m.intr_vec; S->intr_prio = m.intr_prio;
                S->intr_eoi_enviado = m.intr_eoi_enviado;
                S->pic.en_servicio = S->pic.n_isr ? S->pic.pila_isr[S->pic.n_isr - 1] : -1;
                snprintf(buf, sizeof buf, "Reanudando ISR externo de %s (paso %d/7).",
                         DEV_NOMBRE[m.cur_dev], m.stage);
                logmsg(S, "SYS", buf);
                return;
            }
            S->in_isr = 0; S->stage = 0;
            S->cur_dev = -1; S->intr_dev = -1;
            return;
        }
    }

    /* ============ Flujo normal (sin ISR) ============ */

    /* Timer: fin de quantum → IRQ de mayor prioridad. */
    S->timer--;
    if (S->timer <= 0) {
        pic_alzar(S, DEV_TIMER);
        S->timer = S->quantum;
        logmsg(S, "IRQ", "Timer: quantum agotado → IRQ.");
    }

    /* Planificación: la CPU toma un proceso listo. */
    if (S->cpu < 0 && S->n_ready > 0) {
        S->cpu = ready_shift(S);
        S->proc[S->cpu].estado = PROC_EJECUTANDO;
        if (S->proc[S->cpu].restante <= 0) {
            Paso ph = PLAN[S->cpu][S->proc[S->cpu].fase];
            S->proc[S->cpu].restante = (ph.tipo == 0) ? ph.arg : 1;
        }
        snprintf(buf, sizeof buf, "CPU toma a P%d.", S->cpu + 1);
        logmsg(S, "SYS", buf);
    }

    /* Ejecución de una instrucción de la ráfaga de CPU. */
    if (S->cpu >= 0) {
        int me = S->cpu;              /* índice del proceso en CPU (se usa tras liberar la CPU) */
        Proceso *p = &S->proc[me];
        S->pc += 4; S->r0 = (S->r0 + 1) % 16; S->busy++; p->restante--;
        if (p->restante <= 0) {
            int fase = p->fase;
            Paso ph = PLAN[me][fase];
            if (ph.tipo == 0) { /* terminó ráfaga de CPU → avanza a la E/S */
                fase = (fase + 1) % N_PASOS; p->fase = fase; ph = PLAN[me][fase];
            }
            if (ph.tipo == 1) { /* inicia E/S: driver + bloqueo */
                int d = ph.arg;
                S->driver_busy = 4; S->driver_dev = d;
                snprintf(buf, sizeof buf, "Paso 1 — driver inicia E/S (read) para P%d en %s.",
                         me + 1, DEV_NOMBRE[d]);
                logmsg(S, "DRV", buf);
                p->estado = PROC_BLOQUEADO; p->dispositivo = d;
                S->dev[d].cola[S->dev[d].n_cola++] = me;
                snprintf(buf, sizeof buf, "Paso 2 — CPU → %s controller: initiates I/O.", DEV_NOMBRE[d]);
                logmsg(S, "DRV", buf);
                S->ctx++; S->cpu = -1;
                fase = (fase + 1) % N_PASOS; p->fase = fase;
                p->restante = (PLAN[me][fase].tipo == 0) ? PLAN[me][fase].arg : 2;
            }
        }
    }

    /* Dispositivos: ejecutan la E/S en paralelo con su propio reloj. */
    for (int d = 0; d < N_DISPOS; d++) {
        if (d == DEV_TIMER) continue; /* el Timer no se sirve por E/S */
        Dispositivo *D = &S->dev[d];
        if (D->sirviendo < 0 && D->n_cola > 0) {
            D->sirviendo = D->cola[0];
            for (int i = 1; i < D->n_cola; i++) D->cola[i - 1] = D->cola[i];
            D->n_cola--;
            D->restante = DEV_SERV[d] > 0 ? DEV_SERV[d] : 1;
            D->reloj = 0;
        }
        if (D->sirviendo >= 0) {
            D->reloj++;
            int paso = S->t_asincrono ? (D->reloj % 2 == 0 ? 1 : 0) : 1; /* reloj propio */
            D->restante -= paso;
            if (D->restante <= 0) {
                int pid = D->sirviendo;
                D->sirviendo = -1;
                S->proc[pid].pendiente = d;
                pic_alzar(S, d);
                snprintf(buf, sizeof buf, "Paso 4 — %s controller: E/S completa → IRQ (libera a P%d).",
                         DEV_NOMBRE[d], pid + 1);
                logmsg(S, "IRQ", buf);
            }
        }
    }

    /* Arbitraje del PIC: acepta la IRQ de mayor prioridad y entra al ISR. */
    int cand = pic_arbitrar(S);
    if (cand >= 0) {
        int alzada = S->irq_alzada_en[cand] < 0 ? S->ciclo : S->irq_alzada_en[cand];
        pic_consumir(S, cand);
        S->pic.isr[cand] = 1;
        S->pic.pila_isr[S->pic.n_isr++] = cand;
        S->pic.en_servicio = cand;
        S->intr_dev = cand; S->intr_vec = DEV_VEC[cand]; S->intr_prio = DEV_PRIO[cand];
        S->intr_eoi_enviado = 0;
        S->cur_dev = cand; S->ifbit = 0; S->in_isr = 1; S->stage = 1; S->irq++;
        S->lat_sum += (S->ciclo - alzada) > 0 ? (S->ciclo - alzada) : 1;
        snprintf(buf, sizeof buf, "Paso 5 — IRQ de %s aceptada (vec 0x%02X, prio %d).",
                 DEV_NOMBRE[cand], DEV_VEC[cand], DEV_PRIO[cand]);
        logmsg(S, "IRQ", buf);
    }

    /* El driver termina su trabajo (BUG 14). */
    if (S->driver_busy > 0) { S->driver_busy--; if (S->driver_busy == 0) S->driver_dev = -1; }

    /* Comprobación de integridad (BUG 15): ningún proceso debe quedar bloqueado
     * sin dispositivo asociado ni E/S pendiente. */
    for (int id = 0; id < N_PROC; id++) {
        Proceso *p = &S->proc[id];
        if (p->estado == PROC_BLOQUEADO && p->pendiente < 0 && p->dispositivo < 0) {
            snprintf(buf, sizeof buf, "P%d bloqueado sin dispositivo → devuelto a LISTO.", id + 1);
            logmsg(S, "SYS", buf);
            p->estado = PROC_LISTO;
            ready_push(S, id);
        }
    }
}
