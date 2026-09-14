# Arquitectura y documentación técnica

Documento para desarrolladores del equipo: cómo está organizado el código, cómo
funciona el motor, qué correcciones se hicieron y cómo probar/extender.

> Para **instalar, compilar y usar**, ver [`MANUAL.md`](MANUAL.md).

---

## 1. Estructura del repositorio (aporte de esta rama)

```
include/
  config_sim.h      Parámetros del modelo: dispositivos, prioridades, vectores, quantum.
  simulador.h       Estado completo (struct Simulador) y API pública del motor.
src/
  nucleo/
    simulador.c     EL MOTOR: raise/arbitraje del PIC, contexto, tick() del ciclo.
    main_sim.c      Modo consola/traza (CLI, escribe trace.csv).
  ui/
    main_gui.c      Interfaz gráfica interactiva con raylib.
tests/
  test_interrupciones.c   Pruebas de invariantes teóricos.
docs/
  MANUAL.md         Manual de usuario.
  ARQUITECTURA.md   Este documento.
  NUCLEO_WEB.md     Resumen del port y correcciones.
Makefile            Targets: gui (por defecto), cli, test, run, clean.
```

El **motor** (`simulador.c`) no depende de la interfaz. Tanto la GUI (raylib)
como el modo consola llaman a las **mismas** funciones `sim_init()` y
`sim_tick()`. Eso garantiza que lo que se ve en pantalla y lo que se prueba es
exactamente la misma lógica.

---

## 2. El modelo teórico

Se simula el ciclo de E/S dirigida por interrupciones con:

- **CPU** con tres procesos y planificación por **cola de listos**. Cada proceso
  alterna ráfagas de CPU y operaciones de E/S según un *plan* fijo.
- **PIC 8259A simplificado**, con tres registros de un bit por dispositivo:
  - **IRR** (*Interrupt Request Register*): líneas pendientes.
  - **IMR** (*Interrupt Mask Register*): líneas enmascaradas.
  - **ISR** (*In-Service Register*): interrupciones en atención.
  - **Prioridad fija**: menor número = mayor prioridad
    (`Timer=0 > Disco=1 > Teclado=2`).
- **IVT** (tabla de vectores): cada dispositivo tiene su vector (`0x20..0x22`) y
  su rutina `isr_<dev>()`.
- **Contexto**: se guarda/restaura PC, registros, flags y **modo**
  (usuario/kernel) en una pila, lo que permite el **anidamiento**.
- **Timer** (quantum): fuente de mayor prioridad que provoca el cambio de proceso
  (planificación). No forma parte de la Figura 1.4.
- **Dispositivos** (Disco, Teclado) con **reloj propio** para modelar la E/S
  **asíncrona**.

Configuración en `config_sim.h`:

```c
DEV_PRIO = { 0, 1, 2 };        // Timer, Disco, Teclado
DEV_VEC  = { 0x20, 0x21, 0x22 };
DEV_SERV = { 0, 6, 4 };        // ciclos de servicio (Timer no se sirve por E/S)
QUANTUM  = 4;
```

---

## 3. El estado: `struct Simulador`

Definido en `include/simulador.h`. Campos principales:

- **CPU**: `cpu` (proceso en ejecución o −1), `modo`, `pc`, `r0`, `eflags`,
  `ifbit`.
- **Cola de listos**: `ready[]`, `n_ready`.
- **Procesos**: `proc[3]` con `{fase, restante, estado, dispositivo, pendiente}`.
- **Dispositivos**: `dev[]` con `{sirviendo, restante, reloj, cola[]}`.
- **PIC**: `pic.{irr, imr, isr, irr_count, pila_isr, en_servicio}`.
- **Ciclo de interrupción**: `in_isr`, `stage` (1..7), `cur_dev`, `intr_*`,
  `pila_ctx[]` (contextos), `pila_nest[]` (ISR externos preemptados).
- **Métricas**: `irq, ctx, busy, es, lat_sum, nest, eoi`.
- **Bitácora**: `logtxt[]/logtag[]/logt[]` (para la GUI).
- **Toggles**: `t_anidar, t_eoi_temprano, t_asincrono`.

---

## 4. El ciclo: `sim_tick()`

Cada llamada avanza **un ciclo** discreto. Estructura:

**Si hay un ISR en curso (`in_isr`):**
1. Comprueba **anidamiento**: si `anidar` está activo, `IF=1` (hubo `STI`) y hay
   una IRQ de mayor prioridad madura, **preempta**: guarda el ISR externo en
   `pila_nest`, empieza el interno.
2. Avanza la etapa `stage` (1→7). En cada etapa ocurre lo real:
   - etapa 2: **INTA**.
   - etapa 3: **guardar contexto** (`ctx_guardar`).
   - etapa 4: consultar **IVT** (vector al bus) y, si `anidar`, **STI** (`IF=1`).
   - etapa 5: **EOI temprano** (si el toggle está activo).
   - etapa 6: **aplicar efecto** (liberar el proceso) + **EOI**.
   - etapa 7: **restaurar contexto** (IRET).
3. Al terminar (etapa >7): si había un ISR externo preemptado, lo **reanuda**;
   si no, sale del ISR.

**Si no hay ISR (flujo normal):**
1. **Timer**: descuenta el quantum; al llegar a 0, alza IRQ del Timer.
2. **Planificación**: la CPU toma un proceso listo.
3. **Ejecución**: corre una instrucción; al terminar su ráfaga, si toca E/S
   lanza el **driver**, **bloquea** el proceso y lo encola en el dispositivo.
4. **Dispositivos**: avanzan su servicio con **reloj propio**; al terminar,
   marcan `pendiente` y **alzan IRQ**.
5. **Arbitraje del PIC**: elige la IRQ de mayor prioridad **madura** y no
   enmascarada, y entra al ISR.
6. **Driver**: cuenta atrás y se libera.
7. **Comprobación de integridad**: ningún proceso queda bloqueado sin causa.

Funciones clave: `pic_alzar`, `pic_arbitrar`, `pic_consumir`, `pic_eoi`,
`ctx_guardar`, `ctx_restaurar`, `aplicar_efecto`.

---

## 5. Correcciones incorporadas (respecto a versiones previas)

Estas correcciones vienen validadas del prototipo web y están **desde el inicio**
en el motor en C:

1. **Conteo de IRQs por dispositivo** (`irr_count`): no se pierden solicitudes
   repetidas del mismo dispositivo.
2. **Una IRQ libera exactamente un proceso** (no todos los que esperan).
3–5. El **flujo** y el badge siguen el trabajo real; el ISR del **Timer** se
   distingue de una E/S.
6. El **arbitraje espera ≥1 ciclo** tras alzar la IRQ (maduración).
7. **Reloj propio del dispositivo** para la E/S asíncrona.
8. **Restauración correcta del modo** usuario/kernel.
9. **Anidamiento real** con `STI` (antes era imposible: el ciclo retornaba antes
   de poder anidar).
14. El **driver se libera** al terminar su trabajo.
15. **Comprobación de integridad** de procesos bloqueados.

> Además, durante el port se detectó y corrigió un bug propio: al anidar no se
> guardaba/restauraba el estado del **EOI** del ISR externo, lo que dejaba el
> `en_servicio` "pegado" y estancaba la simulación. Se resolvió guardando todo el
> estado de la interrupción en el marco de anidamiento.

---

## 6. Pruebas

```bash
make test
```

`tests/test_interrupciones.c` verifica los invariantes teóricos:

1. **Una** IRQ de un dispositivo con varios procesos en espera libera **uno solo**.
2. **Dos** IRQs del mismo dispositivo liberan **dos** procesos (no se pierde la
   segunda).
3. En **3000 ciclos** ningún proceso queda bloqueado sin dispositivo ni sufre
   **inanición** (cada proceso se libera decenas de veces).
4. Con **anidar** activo ocurren **preempciones reales** (el contador de
   anidamientos crece).

Resultado esperado: `TODAS LAS PRUEBAS PASARON.`

---

## 7. Cómo extender

- **Agregar un dispositivo**: en `config_sim.h` sube `N_DISPOS`, añade su nombre,
  prioridad, vector y tiempo de servicio; en `simulador.c` inclúyelo en el lazo
  de dispositivos y en los planes de proceso.
- **Cambiar tiempos**: ajusta `DEV_SERV`, `QUANTUM` o los planes `PLAN[][]` en
  `simulador.c`.
- **Nuevos datos en la traza**: añade columnas en `main_sim.c`
  (`escribir_cabecera` / `escribir_fila`).
- **Nueva vista en la GUI**: añade un panel al arreglo de `main_gui.c` (base,
  nombre, visibilidad) y su función de dibujo.

---

## 8. Nota de alcance

Este subsistema **excede** el alcance acordado en `WORKPLAN.md`
(un dispositivo, sin timer, prioridades, máscaras, anidamiento ni scheduler).
Por eso vive en la rama `feature/simulador-nucleo` y **no** se ha integrado a
`main`. Su incorporación debe acordarse con el equipo mediante Pull Request.
