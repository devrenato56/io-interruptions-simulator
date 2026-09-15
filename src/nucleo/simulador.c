/* Ciclo de E/S: driver -> transferencia -> IRQ -> ISR -> EOI -> IRET.
 * Cada tick avanza los dispositivos incluso durante una ISR. La CPU tiene
 * un unico flujo; su contexto se conserva antes de deshabilitar IRQ. */
#include <stdio.h>
#include <string.h>
#include "simulador.h"

int sim_verboso = 0;
const char *const DEV_NOMBRE[N_DISPOS] = { "Disco", "Teclado" };
const int DEV_PRIO[N_DISPOS] = { 0, 1 };
const int DEV_VEC[N_DISPOS] = { 0x21, 0x22 };
const int DEV_SERV[N_DISPOS] = { 6, 4 };

static void logmsg(Simulador *S, const char *etq, const char *msg) {
    if (sim_verboso) printf("  t%-4d [%s] %s\n", S->ciclo, etq, msg);
    for (int i = SIM_LOG_MAX - 1; i > 0; i--) {
        memcpy(S->logtxt[i], S->logtxt[i - 1], sizeof S->logtxt[0]);
        S->logtag[i] = S->logtag[i - 1];
        S->logt[i] = S->logt[i - 1];
    }
    snprintf(S->logtxt[0], sizeof S->logtxt[0], "%s", msg);
    S->logtag[0] = etq[0] == 'I' ? 1 : etq[0] == 'D' ? 2 : 0;
    S->logt[0] = S->ciclo;
    if (S->nlog < SIM_LOG_MAX) S->nlog++;
}

void sim_mascara(const int reg[N_DISPOS], char out[N_DISPOS + 1]) {
    for (int d = 0; d < N_DISPOS; d++) out[d] = reg[d] ? '1' : '0';
    out[N_DISPOS] = '\0';
}

int sim_solicitar_es(Simulador *S, int d) {
    if (!S || d < 0 || d >= N_DISPOS) return -1;
    Dispositivo *D = &S->dev[d];
    if (D->n_cola + D->n_pendientes + (D->sirviendo >= 0) >= MAX_SOLICITUDES)
        return -1;
    int id = S->siguiente_solicitud++;
    D->cola[D->n_cola++] = id;
    S->driver_busy = 4;
    S->driver_dev = d;
    char buf[100];
    snprintf(buf, sizeof buf, "Driver -> %s: inicia E/S #%d.", DEV_NOMBRE[d], id);
    logmsg(S, "DRV", buf);
    return id;
}

static void avanzar_dispositivos(Simulador *S) {
    for (int d = 0; d < N_DISPOS; d++) {
        Dispositivo *D = &S->dev[d];
        if (D->sirviendo < 0 && D->n_cola > 0) {
            D->sirviendo = D->cola[0];
            for (int i = 1; i < D->n_cola; i++) D->cola[i - 1] = D->cola[i];
            D->n_cola--;
            D->restante = DEV_SERV[d];
            D->reloj = 0;
        }
        if (D->sirviendo < 0) continue;
        D->reloj++;
        D->restante -= S->t_asincrono ? D->reloj % 2 == 0 : 1;
        if (D->restante > 0) continue;
        D->pendientes[D->n_pendientes++] = (FinalizacionES){D->sirviendo, S->ciclo};
        S->pic.irr_count[d]++;
        S->pic.irr[d] = 1;
        if (S->irq_alzada_en[d] < 0) S->irq_alzada_en[d] = S->ciclo;
        char buf[100];
        snprintf(buf, sizeof buf, "%s: transferencia #%d completa -> IRQ.",
                 DEV_NOMBRE[d], D->sirviendo);
        logmsg(S, "IRQ", buf);
        D->sirviendo = -1;
    }
}

static int pic_arbitrar(const Simulador *S) {
    if (!S->ifbit) return -1;
    int mejor = -1;
    for (int d = 0; d < N_DISPOS; d++) {
        if (!S->pic.irr[d] || S->pic.imr[d]) continue;
        if (S->irq_alzada_en[d] < 0 || S->ciclo <= S->irq_alzada_en[d]) continue;
        /* La politica de anidamiento sigue a la ISR activa aun con EOI temprano. */
        if (S->in_isr && (!S->t_anidar || S->stage < 4 || S->stage >= 7 ||
                         DEV_PRIO[d] >= S->intr_prio)) continue;
        if (mejor < 0 || DEV_PRIO[d] < DEV_PRIO[mejor]) mejor = d;
    }
    return mejor;
}

static void pic_eoi(Simulador *S) {
    int d = S->intr_dev;
    S->pic.isr[d] = 0;
    int w = 0;
    for (int i = 0; i < S->pic.n_isr; i++)
        if (S->pic.pila_isr[i] != d) S->pic.pila_isr[w++] = S->pic.pila_isr[i];
    S->pic.n_isr = w;
    S->pic.en_servicio = w ? S->pic.pila_isr[w - 1] : -1;
    S->intr_eoi_enviado = 1;
    S->eoi++;
    logmsg(S, "SYS", "EOI enviado al PIC.");
}

static void entrar_isr(Simulador *S, int d) {
    char buf[100];
    if (S->in_isr) {
        S->pila_nest[S->n_nest++] = (MarcoISR){
            S->intr_dev, S->intr_vec, S->intr_prio, S->intr_eoi_enviado,
            S->stage, S->cur_dev
        };
        S->nest++;
        snprintf(buf, sizeof buf, "[ANIDADA] %s interrumpe ISR de %s.",
                 DEV_NOMBRE[d], DEV_NOMBRE[S->intr_dev]);
        logmsg(S, "IRQ", buf);
    }
    /* Captura atomica del estado anterior a la entrada, incluido IF. */
    S->pila_ctx[S->n_ctx++] = (MarcoContexto){S->pc, S->r0, S->eflags, S->ifbit, S->modo};
    S->contextos_guardados++;
    S->ifbit = 0;
    S->eflags &= ~0x0200u;
    S->modo = 1;
    S->lat_sum += S->ciclo - S->irq_alzada_en[d];
    S->pic.irr_count[d]--;
    S->pic.irr[d] = S->pic.irr_count[d] > 0;
    /* La primera finalizacion pertenece ahora a esta ISR; sigue en la cola
     * hasta atenderla, por lo que la siguiente IRQ corresponde al indice 1. */
    S->irq_alzada_en[d] = S->pic.irr[d] ? S->dev[d].pendientes[1].ciclo_irq : -1;
    S->pic.isr[d] = 1;
    S->pic.pila_isr[S->pic.n_isr++] = d;
    S->pic.en_servicio = d;
    S->intr_dev = S->cur_dev = d;
    S->intr_vec = DEV_VEC[d];
    S->intr_prio = DEV_PRIO[d];
    S->intr_eoi_enviado = 0;
    S->in_isr = 1;
    S->stage = 1;
    S->irq++;
    snprintf(buf, sizeof buf, "IRQ de %s aceptada (vector 0x%02X).", DEV_NOMBRE[d], DEV_VEC[d]);
    logmsg(S, "IRQ", buf);
}

static void aplicar_efecto(Simulador *S) {
    Dispositivo *D = &S->dev[S->intr_dev];
    D->ultima_atendida = D->pendientes[0].id;
    for (int i = 1; i < D->n_pendientes; i++) D->pendientes[i - 1] = D->pendientes[i];
    D->n_pendientes--;
    D->completadas++;
    S->es++;
    char buf[100];
    snprintf(buf, sizeof buf, "ISR %s: resultado de E/S #%d atendido.",
             DEV_NOMBRE[S->intr_dev], D->ultima_atendida);
    logmsg(S, "SYS", buf);
}

void sim_init(Simulador *S) {
    memset(S, 0, sizeof *S);
    S->pc = 0x0400;
    S->eflags = 0x0202;
    S->ifbit = 1;
    S->siguiente_solicitud = 1;
    S->demo_dispositivo = DEV_TECLADO;
    for (int d = 0; d < N_DISPOS; d++) {
        S->dev[d].sirviendo = -1;
        S->dev[d].ultima_atendida = -1;
        S->irq_alzada_en[d] = -1;
    }
    S->pic.en_servicio = S->cur_dev = S->intr_dev = S->driver_dev = -1;
    S->t_asincrono = S->t_demo = 1;
}

void sim_tick(Simulador *S) {
    S->ciclo++;
    if (S->driver_busy > 0 && --S->driver_busy == 0) S->driver_dev = -1;
    avanzar_dispositivos(S);

    if (S->in_isr) {
        int d = pic_arbitrar(S);
        if (d >= 0) { entrar_isr(S, d); return; }
        if (S->stage < N_ETAPAS) {
            S->stage++;
            switch (S->stage) {
                case 2: logmsg(S, "SYS", "INTA: reconocimiento de la CPU al PIC."); break;
                case 3: logmsg(S, "SYS", "Contexto preservado: PC, R0, EFLAGS, IF y modo."); break;
                case 4:
                    S->pc = (unsigned)S->intr_vec * 0x100u;
                    logmsg(S, "SYS", "IVT: vector de E/S -> direccion de la ISR.");
                    if (S->t_anidar) {
                        S->ifbit = 1;
                        S->eflags |= 0x0200u;
                        logmsg(S, "SYS", "ISR hace STI: IF=1.");
                    }
                    break;
                case 5:
                    if (S->t_eoi_temprano) pic_eoi(S);
                    break;
                case 6:
                    aplicar_efecto(S);
                    if (!S->intr_eoi_enviado) pic_eoi(S);
                    break;
                case 7: {
                    MarcoContexto c = S->pila_ctx[--S->n_ctx];
                    S->pc = c.pc; S->r0 = c.r0; S->eflags = c.eflags;
                    S->ifbit = c.ifbit; S->modo = c.modo;
                    logmsg(S, "SYS", "IRET: restaurado el mismo flujo interrumpido.");
                    break;
                }
            }
        } else if (S->n_nest > 0) {
            MarcoISR m = S->pila_nest[--S->n_nest];
            S->intr_dev = m.intr_dev; S->intr_vec = m.intr_vec;
            S->intr_prio = m.intr_prio; S->intr_eoi_enviado = m.intr_eoi_enviado;
            S->stage = m.stage; S->cur_dev = m.cur_dev;
            logmsg(S, "SYS", "Reanudando ISR externa.");
        } else {
            S->in_isr = S->stage = 0;
            S->cur_dev = S->intr_dev = -1;
        }
        return;
    }

    /* Instruccion completa del unico flujo, seguida por la consulta de IRQ. */
    S->pc += 4;
    S->r0 = (S->r0 + 1) % 16;
    S->busy++;
    if (S->t_demo && ++S->demo_instrucciones >= 3) {
        if (sim_solicitar_es(S, S->demo_dispositivo) >= 0) {
            S->demo_instrucciones = 0;
            S->demo_dispositivo = (S->demo_dispositivo + 1) % N_DISPOS;
        }
    }
    int d = pic_arbitrar(S);
    if (d >= 0) entrar_isr(S, d);
}
