/* Regresiones del ciclo de E/S: solicitudes reales, PIC y retorno a CPU. */
#include <stdio.h>
#include <string.h>
#include "simulador.h"

static int fallos;
static void check(int cond, const char *msg) {
    printf("  [%s] %s\n", cond ? "OK" : "FALLO", msg);
    if (!cond) fallos++;
}
static void ticks(Simulador *S, int n) {
    for (int i = 0; i < n; i++) sim_tick(S);
}
static void init_manual(Simulador *S) {
    sim_init(S);
    S->t_demo = 0;
    S->t_asincrono = 0;
}
static int vacio(const Simulador *S) {
    if (S->in_isr) return 0;
    for (int d = 0; d < N_DISPOS; d++)
        if (S->dev[d].sirviendo >= 0 || S->dev[d].n_cola ||
            S->dev[d].n_pendientes || S->pic.irr_count[d]) return 0;
    return 1;
}
static void drenar(Simulador *S) {
    for (int i = 0; i < 2000 && !vacio(S); i++) sim_tick(S);
    check(vacio(S), "todas las solicitudes terminan y la CPU sale de ISR");
}
static void test_sin_es(void) {
    Simulador S; init_manual(&S);
    ticks(&S, 1000);
    check(S.irq == 0 && S.eoi == 0 && S.es == 0 && S.n_ctx == 0,
          "sin E/S no aparecen interrupciones");
    check(S.pc == 0x400 + 4000 && S.busy == 1000, "el flujo de CPU avanza sin E/S");
}
static void test_fifo_y_mascaras(void) {
    Simulador S; init_manual(&S);
    S.pic.imr[DEV_DISCO] = 1;
    int a = sim_solicitar_es(&S, DEV_DISCO);
    int b = sim_solicitar_es(&S, DEV_DISCO);
    int c = sim_solicitar_es(&S, DEV_DISCO);
    ticks(&S, DEV_SERV[DEV_DISCO] * 3);
    check(S.irq == 0 && S.dev[DEV_DISCO].n_pendientes == 3 &&
          S.pic.irr_count[DEV_DISCO] == 3, "IMR retiene las tres IRQ sin perder transferencias");
    check(S.dev[DEV_DISCO].pendientes[0].id == a &&
          S.dev[DEV_DISCO].pendientes[1].id == b &&
          S.dev[DEV_DISCO].pendientes[2].id == c, "identidad y orden FIFO de solicitudes");
    S.pic.imr[DEV_DISCO] = 0;
    for (int i = 0; i < 20 && S.es < 1; i++) sim_tick(&S);
    check(S.es == 1 && S.dev[DEV_DISCO].ultima_atendida == a &&
          S.dev[DEV_DISCO].n_pendientes == 2 && S.pic.irr_count[DEV_DISCO] == 2,
          "una ISR atiende exactamente una solicitud");
    drenar(&S);
    check(S.es == 3 && S.irq == 3 && S.eoi == 3 &&
          S.dev[DEV_DISCO].ultima_atendida == c, "IRQs repetidas se atienden una sola vez");
}
static void test_if_y_contexto(void) {
    Simulador S; init_manual(&S);
    S.ifbit = 0; S.eflags &= ~0x200u;
    sim_solicitar_es(&S, DEV_TECLADO);
    ticks(&S, 20);
    check(S.irq == 0 && S.pic.irr_count[DEV_TECLADO] == 1, "IF=0 conserva la IRQ sin aceptarla");
    S.ifbit = 1; S.eflags |= 0x200u;
    unsigned pc = S.pc + 4, flags = S.eflags;
    int r0 = (S.r0 + 1) % 16;
    sim_tick(&S);
    check(S.in_isr && S.n_ctx == 1 && !S.ifbit &&
          S.pila_ctx[0].ifbit == 1 && S.pila_ctx[0].pc == pc,
          "entrada guarda el contexto entre instrucciones antes de limpiar IF");
    while (S.in_isr && S.stage < 7) sim_tick(&S);
    check(S.pc == pc && S.r0 == r0 && S.eflags == flags &&
          S.ifbit == 1 && S.modo == 0 && S.n_ctx == 0,
          "IRET restaura PC, R0, flags, IF y modo del mismo flujo");
    sim_tick(&S);
    sim_tick(&S);
    check(S.pc == pc + 4 && S.r0 == (r0 + 1) % 16, "CPU continua desde la siguiente instruccion");
}
static void test_prioridad_y_maduracion(void) {
    Simulador S; init_manual(&S);
    S.pic.imr[DEV_DISCO] = S.pic.imr[DEV_TECLADO] = 1;
    sim_solicitar_es(&S, DEV_DISCO);
    sim_solicitar_es(&S, DEV_TECLADO);
    ticks(&S, DEV_SERV[DEV_DISCO]);
    S.pic.imr[DEV_DISCO] = S.pic.imr[DEV_TECLADO] = 0;
    sim_tick(&S);
    check(S.cur_dev == DEV_DISCO && S.intr_vec == DEV_VEC[DEV_DISCO],
          "dos IRQ maduras: Disco gana por prioridad y conserva su vector");
    drenar(&S);
    check(S.dev[DEV_TECLADO].completadas == 1, "Teclado se atiende despues de Disco");
    init_manual(&S);
    sim_solicitar_es(&S, DEV_TECLADO);
    ticks(&S, DEV_SERV[DEV_TECLADO]);
    check(S.irq == 0 && S.pic.irr[DEV_TECLADO], "IRQ espera un ciclo antes de ser aceptada");
    sim_tick(&S);
    check(S.irq == 1 && S.lat_sum == 1, "latencia medida desde la finalizacion real");
}
static void test_anidamiento_real(int temprano, int habilitado) {
    Simulador S; init_manual(&S);
    S.t_anidar = habilitado; S.t_eoi_temprano = temprano;
    sim_solicitar_es(&S, DEV_TECLADO);
    ticks(&S, 2);
    sim_solicitar_es(&S, DEV_DISCO);
    ticks(&S, 3); /* IRQ teclado aceptada en t5; disco completa en t8 */
    unsigned pc = S.pila_ctx[0].pc, flags = S.pila_ctx[0].eflags;
    int r0 = S.pila_ctx[0].r0;
    ticks(&S, 3); /* STI en t8 */
    check(S.pic.irr[DEV_DISCO] && S.dev[DEV_DISCO].n_pendientes == 1,
          "disco genera IRQ durante la ISR de teclado");
    if (temprano) {
        S.pic.imr[DEV_DISCO] = 1;
        sim_tick(&S); /* etapa 5: el PIC ya recibio EOI del teclado */
        S.pic.imr[DEV_DISCO] = 0;
    }
    sim_tick(&S);
    check(habilitado ? (S.cur_dev == DEV_DISCO && S.n_ctx == 2 && S.n_nest == 1)
                     : (S.cur_dev == DEV_TECLADO && S.n_nest == 0),
          "la opcion anidar controla la preempcion por prioridad");
    drenar(&S);
    check(S.nest == habilitado && S.irq == 2 && S.es == 2 && S.eoi == 2 &&
          !S.pic.n_isr && !S.n_ctx && !S.n_nest && S.pic.en_servicio == -1,
          "ISR anidadas y EOI temprano no duplican ni dejan servicio pendiente");
    /* Sin anidamiento hay una instruccion entre ambas ISR. */
    check(S.pc == pc + (habilitado ? 0u : 4u) &&
          S.r0 == (r0 + (habilitado ? 0 : 1)) % 16 &&
          S.eflags == flags && S.ifbit == 1 && S.modo == 0,
          "retorno anidado conserva el contexto externo y el flujo principal");
}
static void test_limites(void) {
    Simulador S; init_manual(&S);
    S.pic.imr[DEV_DISCO] = 1;
    for (int i = 0; i < MAX_SOLICITUDES; i++)
        if (sim_solicitar_es(&S, DEV_DISCO) < 0) { check(0, "capacidad de cola"); break; }
    Simulador antes = S;
    check(sim_solicitar_es(&S, DEV_DISCO) == -1 &&
          sim_solicitar_es(&S, -1) == -1 && sim_solicitar_es(&S, N_DISPOS) == -1 &&
          sim_solicitar_es(NULL, DEV_DISCO) == -1 &&
          memcmp(&antes, &S, sizeof S) == 0, "cola llena y dispositivo invalido no modifican estado");
    ticks(&S, MAX_SOLICITUDES * DEV_SERV[DEV_DISCO] + 1);
    check(S.dev[DEV_DISCO].n_pendientes == MAX_SOLICITUDES &&
          sim_solicitar_es(&S, DEV_DISCO) == -1, "limite incluye transferencias que esperan ISR");
    S.pic.imr[DEV_DISCO] = 0;
    drenar(&S);
    check(S.es == MAX_SOLICITUDES && sim_solicitar_es(&S, DEV_DISCO) > 0,
          "la capacidad se recupera al atender las solicitudes");
}
static void test_corrida_larga(void) {
    for (int opciones = 0; opciones < 8; opciones++) {
        Simulador S; sim_init(&S);
        S.t_anidar = opciones & 1;
        S.t_eoi_temprano = (opciones >> 1) & 1;
        S.t_asincrono = (opciones >> 2) & 1;
        int integridad = 1;
        for (int t = 0; t < 3000; t++) {
            sim_tick(&S);
            int total = S.es;
            for (int d = 0; d < N_DISPOS; d++) {
                Dispositivo *D = &S.dev[d];
                int pendientes = D->n_cola + D->n_pendientes + (D->sirviendo >= 0);
                total += pendientes;
                if (pendientes > MAX_SOLICITUDES || pendientes < 0 ||
                    S.pic.irr[d] != (S.pic.irr_count[d] > 0)) integridad = 0;
            }
            if (total != S.siguiente_solicitud - 1 ||
                S.n_ctx < 0 || S.n_ctx > N_DISPOS ||
                S.n_nest < 0 || S.n_nest >= N_DISPOS ||
                S.ifbit != !!(S.eflags & 0x200u)) integridad = 0;
        }
        S.t_demo = 0;
        drenar(&S);
        check(integridad && S.es == S.siguiente_solicitud - 1 && S.irq == S.es &&
              S.eoi == S.irq && S.contextos_guardados == S.irq &&
              S.dev[DEV_DISCO].completadas > 10 && S.dev[DEV_TECLADO].completadas > 10,
              "3000 ciclos: identidad, limites y progreso con todas las variantes");
    }
}
int main(void) {
    test_sin_es();
    test_fifo_y_mascaras();
    test_if_y_contexto();
    test_prioridad_y_maduracion();
    for (int temprano = 0; temprano <= 1; temprano++)
        for (int habilitado = 0; habilitado <= 1; habilitado++)
            test_anidamiento_real(temprano, habilitado);
    test_limites();
    test_corrida_larga();
    printf("\n%s\n", fallos ? "PRUEBAS FALLIDAS" : "TODAS LAS PRUEBAS PASARON.");
    return fallos ? 1 : 0;
}
