#!/usr/bin/env python3
"""
animacion.py — Visualización/animación del ciclo de E/S dirigida por
interrupciones (Figura 1.4) a partir de la traza CSV que emite el simulador en C.

Es el equivalente local (sin navegador) del simulador web: lee trace.csv,
ilumina los 7 nodos de la figura según el estado de cada ciclo y muestra un
panel con CPU, PIC, dispositivos y métricas.

Uso:
    python viz/animacion.py                      # anima trace.csv en pantalla
    python viz/animacion.py --entrada trace.csv  # otra traza
    python viz/animacion.py --guardar salida.gif # exporta a GIF (requiere pillow)
    python viz/animacion.py --fps 6              # velocidad

Requisitos: matplotlib (y pillow solo si se exporta a GIF).
"""
import argparse
import csv
import matplotlib
import matplotlib.pyplot as plt
import matplotlib.patches as mpatches
from matplotlib.animation import FuncAnimation

# Paleta (coherente con el simulador web).
FONDO   = "#0f1a24"
TINTA   = "#e9f1f8"
TENUE   = "#93a9bb"
ACTIVO  = "#f0b64d"   # nodo/flecha activa
CPU_BG  = "#0b141d"
IO_BG   = "#7cc3e8"
IO2_BG  = "#c8d0d8"
HND_BG  = "#bfe0f2"
BORDE   = "#2c4356"

# Definición de los 7 nodos: (x, y, w, h, texto, color_base).
NODOS = {
    1: (0.05, 0.80, 0.40, 0.12, "1 · device driver\ninitiates I/O", CPU_BG),
    2: (0.55, 0.80, 0.40, 0.12, "2 · initiates I/O\n(controller)", IO_BG),
    3: (0.55, 0.62, 0.40, 0.14, "3 · input ready / output\ncomplete / error →\ngenera IRQ", IO2_BG),
    4: (0.05, 0.44, 0.40, 0.14, "4 · CPU recibe IRQ,\ntransfiere control\nal handler", CPU_BG),
    5: (0.05, 0.26, 0.40, 0.14, "5 · el handler procesa\ndatos, retorna (IRET)", HND_BG),
    6: (0.05, 0.08, 0.40, 0.12, "6 · la CPU reanuda\nla tarea interrumpida", CPU_BG),
}
# Texto oscuro para nodos de relleno claro (2,3,5); claro para los de CPU.
TEXTO_OSCURO = {2, 3, 5}


def leer_traza(ruta):
    with open(ruta, newline="") as f:
        return list(csv.DictReader(f))


def nodos_activos(fila):
    """Replica la lógica del web para saber qué nodos iluminar en este ciclo."""
    in_isr   = fila["in_isr"] == "1"
    stage    = int(fila["stage"])
    cur      = fila["cur_dev"]
    irr      = fila["irr"]           # orden: Timer, Disco, Teclado
    drv      = int(fila["driver_busy"]) > 0
    sirv     = int(fila["disco_serv"]) > 0 or int(fila["teclado_serv"]) > 0
    dev_irq  = irr[1] == "1" or irr[2] == "1"
    isr_dev  = in_isr and cur not in ("-", "Timer")

    act = {n: False for n in range(1, 8)}
    if drv:
        act[1] = act[2] = True
    if sirv:
        act[3] = True
    if dev_irq or (isr_dev and stage <= 2):
        act[4] = True
    if isr_dev and 1 <= stage <= 4:
        act[5] = True
    if isr_dev and 5 <= stage <= 7:
        act[6] = True
    return act


def main():
    ap = argparse.ArgumentParser(description="Animación del ciclo de E/S (Fig. 1.4)")
    ap.add_argument("--entrada", default="trace.csv")
    ap.add_argument("--guardar", default=None, help="archivo .gif de salida")
    ap.add_argument("--fps", type=int, default=6)
    ap.add_argument("--desde", type=int, default=0, help="ciclo inicial")
    args = ap.parse_args()

    filas = leer_traza(args.entrada)[args.desde:]
    if not filas:
        print("Traza vacía.")
        return
    if args.guardar:
        matplotlib.use("Agg")

    fig, (ax, panel) = plt.subplots(
        1, 2, figsize=(12, 7), gridspec_kw={"width_ratios": [1.4, 1]})
    fig.patch.set_facecolor(FONDO)
    for a in (ax, panel):
        a.set_facecolor(FONDO)
        a.set_xlim(0, 1); a.set_ylim(0, 1); a.axis("off")

    ax.set_title("Ciclo de E/S dirigido por interrupciones (Fig. 1.4)",
                 color=TINTA, fontsize=12, fontweight="bold")

    # Dibuja los nodos una sola vez y guarda los parches para recolorearlos.
    parches, textos = {}, {}
    for n, (x, y, w, h, txt, col) in NODOS.items():
        p = mpatches.FancyBboxPatch((x, y), w, h,
                                    boxstyle="round,pad=0.008",
                                    linewidth=1.5, edgecolor=BORDE, facecolor=col)
        ax.add_patch(p)
        parches[n] = p
        tc = "#0d1820" if n in TEXTO_OSCURO else TINTA
        textos[n] = ax.text(x + w / 2, y + h / 2, txt, ha="center", va="center",
                            color=tc, fontsize=8.5)

    # Flechas fijas del recorrido (1→2→3→4→5→6→1).
    flechas = [((0.45, 0.86), (0.55, 0.86)),   # 1→2
               ((0.75, 0.80), (0.75, 0.76)),   # 2→3
               ((0.55, 0.69), (0.45, 0.51)),   # 3→4
               ((0.25, 0.44), (0.25, 0.40)),   # 4→5
               ((0.25, 0.26), (0.25, 0.20)),   # 5→6
               ((0.03, 0.14), (0.03, 0.86))]   # 6→1 (realimentación, paso 7)
    for (x0, y0), (x1, y1) in flechas:
        ax.annotate("", xy=(x1, y1), xytext=(x0, y0),
                    arrowprops=dict(arrowstyle="-|>", color=TENUE, lw=1.4))
    ax.text(0.005, 0.5, "7 · IRET", color=TENUE, fontsize=8, rotation=90, va="center")

    badge = ax.text(0.5, -0.02, "", ha="center", va="top", color=ACTIVO,
                    fontsize=10, fontweight="bold")
    info = panel.text(0.0, 0.98, "", ha="left", va="top", color=TINTA,
                      fontsize=10, family="monospace")

    ETAPAS = ["Driver inicia la E/S", "CPU → controller", "Controlador ejecuta la E/S",
              "Controlador genera la IRQ", "CPU salta al handler", "Handler procesa · EOI",
              "CPU reanuda la tarea"]

    def dibujar(i):
        fila = filas[i]
        act = nodos_activos(fila)
        for n, p in parches.items():
            base = NODOS[n][5]
            if act[n]:
                p.set_edgecolor(ACTIVO); p.set_linewidth(3.2)
            else:
                p.set_edgecolor(BORDE); p.set_linewidth(1.5)
            p.set_facecolor(base)

        in_isr = fila["in_isr"] == "1"
        stage = int(fila["stage"])
        cur = fila["cur_dev"]
        isr_dev = in_isr and cur not in ("-", "Timer")
        if isr_dev:
            paso = stage
        elif fila["irr"][1] == "1" or fila["irr"][2] == "1":
            paso = 4
        elif int(fila["disco_serv"]) > 0 or int(fila["teclado_serv"]) > 0:
            paso = 3
        elif int(fila["driver_busy"]) > 0:
            paso = 1
        else:
            paso = 0
        badge.set_text(f"PASO {paso}/7 · {ETAPAS[paso-1] if paso else 'en espera'}")

        panel_txt = (
            f"ciclo        {fila['ciclo']:>6}\n"
            f"CPU          {'P'+fila['cpu'] if fila['cpu']!='0' else '—':>6}   modo {fila['modo']}\n"
            f"PC           {fila['pc']:>6}   IF={fila['ifbit']}\n"
            f"\n"
            f"PIC   IRR {fila['irr']}   IMR {fila['imr']}   ISR {fila['isr']}\n"
            f"      en servicio: {fila['en_servicio']}\n"
            f"      (orden bits: Timer, Disco, Teclado)\n"
            f"\n"
            f"ISR   {'sí' if in_isr else 'no':<3} etapa {stage}/7   dev {cur}\n"
            f"Timer quantum {fila['timer']}\n"
            f"\n"
            f"Disco    {'atiende P'+fila['disco_serv']+' ('+fila['disco_rem']+')' if fila['disco_serv']!='0' else 'libre'}\n"
            f"Teclado  {'atiende P'+fila['teclado_serv']+' ('+fila['teclado_rem']+')' if fila['teclado_serv']!='0' else 'libre'}\n"
            f"\n"
            f"Procesos  P1={fila['p1']}  P2={fila['p2']}  P3={fila['p3']}   (L/E/B)\n"
            f"\n"
            f"IRQ atend {fila['irq']:>4}   E/S compl {fila['es']:>4}\n"
            f"ctx       {fila['ctx']:>4}   anidam.   {fila['nest']:>4}\n"
            f"EOI       {fila['eoi']:>4}   pend      {fila['pend']:>4}\n"
        )
        info.set_text(panel_txt)
        return list(parches.values()) + [badge, info]

    anim = FuncAnimation(fig, dibujar, frames=len(filas),
                         interval=1000 / args.fps, blit=False, repeat=True)

    if args.guardar:
        anim.save(args.guardar, writer="pillow", fps=args.fps)
        print(f"Animación guardada en {args.guardar}")
    else:
        plt.tight_layout()
        plt.show()


if __name__ == "__main__":
    main()
