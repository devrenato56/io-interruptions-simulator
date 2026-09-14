# Núcleo del simulador (versión completa, portada del web)

> ⚠ **Nota de alcance.** Este módulo **amplía** el alcance declarado en
> `WORKPLAN.md` / `CONTEXT.md` (que se limita a **un** dispositivo, sin timer,
> sin prioridades/máscaras del PIC, sin scheduler ni anidamiento). Se añade en
> la rama `feature/simulador-nucleo` para **no** alterar `main` ni las ramas de
> fase del equipo. Antes de integrarlo a `main` debe acordarse con el equipo,
> porque cambia el alcance del proyecto. Es la versión en C, corregida y
> verificada, del simulador web (`simuladorweb_v4.html`).

## Qué modela

Ciclo de E/S dirigida por interrupciones (Silberschatz, *Operating System
Concepts*, Fig. 1.4) con:

- CPU con tres procesos y planificación por cola de listos.
- PIC 8259A simplificado: **IRR / IMR / ISR** con **prioridad fija** y
  arbitraje con maduración (la IRQ debe llevar ≥1 ciclo).
- Timer (quantum) como fuente de mayor prioridad.
- Dispositivos (Disco, Teclado) con **reloj propio** (E/S asíncrona conmutable).
- Guardado/restauración de contexto (PC, registros, flags y **modo**).
- **Anidamiento real** de interrupciones: con `STI` (toggle "anidar") una IRQ
  de mayor prioridad preempta al ISR en curso y este se reanuda al terminar.

## Correcciones incorporadas (detectadas y verificadas en el web)

1. Conteo de IRQs por dispositivo: no se pierden solicitudes repetidas.
2. Una IRQ libera **exactamente un** proceso.
3. El nodo del flujo sigue el trabajo real del controlador (`serv`).
4. Prioridad del ISR de dispositivo sobre el del driver en la señalización.
5. El ISR del Timer no se confunde con una E/S.
6. El arbitraje espera ≥1 ciclo tras alzar la IRQ.
7. Reloj propio del dispositivo (E/S asíncrona).
8. Restauración correcta del modo usuario/kernel.
9. Anidamiento real con `STI` (antes era imposible).
14. El driver se libera al terminar su trabajo.
15. Comprobación de integridad de procesos bloqueados.

## Interfaz gráfica (Raylib) — el simulador interactivo

La interfaz nativa reproduce el simulador web **en vivo** (cada ciclo se
calcula en tiempo real): escena de hardware (CPU + driver, PIC,
controladores/dispositivos, buses con flechas, cola de listos), osciloscopio
CLK/IRQ/INTA/EOI/DATA, flujo de la Fig. 1.4 con nodos que se iluminan,
bitácora, IVT (clic para enmascarar) y métricas, con controles **Reproducir /
Paso / Reiniciar**, los toggles **anidar / EOI temprano / E/S asíncrona** y un
control de velocidad.

Lienzo tipo CAD: **rueda del ratón = zoom** al cursor, **arrastrar con botón
derecho = mover**, **F = ajustar** todo a la ventana, **R = reiniciar vista**.
La ventana es redimensionable. Cada panel se puede **mover** (arrastrando su
barra de título) y **cerrar** (la **X**); el botón **Config** muestra u oculta
cada vista y reinicia el layout.

Requiere **raylib**. En MSYS2/MinGW-w64 (UCRT64):

```bash
pacman -S mingw-w64-ucrt-x86_64-raylib
make            # compila ./simulador_gui
./simulador_gui
```

En Linux (apt/otro): instala `libraylib-dev` (o compílala) y `make`.

## Simulador de consola / traza (opcional)

Además de la GUI hay un binario sin interfaz que ejecuta N ciclos y emite una
traza CSV (útil para pruebas o análisis):

```bash
make cli
./simulador --ciclos 400 --anidar --salida trace.csv
./simulador --sincrono --verboso        # bitácora por consola
```

Opciones: `--ciclos N`, `--anidar`, `--eoi-temprano`, `--sincrono`,
`--salida ARCH` (o `-` para no escribir), `--verboso`.

## Pruebas

`make test` verifica los invariantes teóricos (idénticos a los del web):
una IRQ libera un proceso; dos IRQs liberan dos (sin pérdida); sin procesos
huérfanos ni inanición en 3000 ciclos; el anidamiento produce preempciones
reales.

## Archivos

```
include/config_sim.h        parámetros del modelo (dispositivos, prioridades, vectores)
include/simulador.h         estado y API del motor
src/nucleo/simulador.c      motor del ciclo de interrupciones
src/nucleo/main_sim.c       binario de consola/traza (opcional)
src/ui/main_gui.c           interfaz gráfica interactiva (Raylib)
tests/test_interrupciones.c pruebas de invariantes
Makefile                    build (gui, cli, test, run)
```
