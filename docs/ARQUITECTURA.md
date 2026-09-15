# Arquitectura del simulador de E/S

## Modelo

Un único flujo de CPU envía solicitudes de lectura a **Disco** y **Teclado**.
Cada dispositivo conserva una cola FIFO de solicitudes y otra de transferencias
finalizadas que esperan atención de su ISR. Una solicitud tiene un ID, no un
estado de proceso.

Se mantienen el PIC con IRR/IMR/ISR, prioridades fijas, máscaras, conteo de IRQ,
vectores, anidamiento con STI, EOI temprano y contexto del flujo interrumpido.
No existen temporizador de interrupciones, quantum ni planificación de procesos.

| Dispositivo | Índice | Prioridad | Vector | Servicio |
|---|---|---|---|---|
| Disco | 0 | 0 (mayor) | 0x21 | 6 pasos |
| Teclado | 1 | 1 | 0x22 | 4 pasos |

Los vectores y prioridades son convenciones didácticas, no un mapeo de hardware
PC real. El PIC incorpora conteo de eventos como simplificación para conservar
una finalización por solicitud; no emula exactamente un 8259A.

## API y solicitudes

- `sim_init(S)`: inicializa el estado y activa la demostración automática.
- `sim_tick(S)`: avanza un ciclo de dispositivos y CPU.
- `sim_solicitar_es(S, d)`: encola una solicitud; devuelve un ID positivo o
  `-1` si el dispositivo no existe o alcanzó su capacidad.
- `sim_mascara(reg, out)`: representa los bits en orden Disco, Teclado.

`t_demo=0` permite generar solicitudes explícitas para pruebas. La demostración
envía una solicitud cada tres instrucciones del mismo flujo, alternando los
dispositivos. Si la cola está llena, reintenta en la siguiente instrucción.
No introduce otra fuente de IRQ.

`MAX_SOLICITUDES` limita el total por dispositivo: en cola, en transferencia
y finalizadas pendientes de ISR. Rechazar una solicitud no altera el estado.
Cada transferencia genera una IRQ y cada ISR retira una sola finalización FIFO.

## Tiempo y CPU

Cada tick avanza los dispositivos incluso si la CPU está atendiendo una ISR.
Con `t_asincrono=1`, el servicio avanza cada dos ticks de su reloj local;
con `0`, cada tick. Estos contadores solo modelan la duración de una E/S.
Un dispositivo inactivo no genera interrupciones por el paso del tiempo.

Fuera de ISR, la CPU completa una instrucción ilustrativa (PC y R0 avanzan)
y consulta el PIC. La IRQ debe tener al menos un ciclo de antigüedad,
estar desenmascarada y encontrar IF habilitado.

## Entrada y retorno de ISR

1. Se acepta la IRQ y se captura atómicamente PC, R0, EFLAGS, IF y modo,
   antes de limpiar IF. Se descuenta una IRQ del IRR.
2. La animación muestra INTA.
3. Se muestra el contexto preservado en la entrada.
4. Se usa el vector del dispositivo para representar la dirección del handler.
   Si está habilitado el anidamiento, STI activa IF.
5. Se emite EOI si está activa la variante temprana.
6. Se atiende una finalización de E/S y se emite EOI si aún falta.
7. IRET restaura el contexto del mismo flujo.

La dirección de ISR y su despacho son representaciones internas del motor;
no se ejecutan instrucciones de hardware ni se usa la IVT del módulo académico.
La captura se hace al aceptar la IRQ para no guardar un IF ya modificado;
la etapa 3 la hace visible en la interfaz.

El anidamiento admite únicamente una prioridad estrictamente mayor que la ISR
activa. Esta política se conserva incluso después de EOI temprano. La pila de
progreso retiene etapa, dispositivo, vector, prioridad y estado del EOI externo.
Al terminar la ISR interna se restaura la externa y después el flujo principal.
El límite de profundidad deriva del número de dispositivos y prioridades.

## Métricas y traza

`contextos_guardados` cuenta entradas a ISR. `busy` cuenta instrucciones del
flujo principal; su porcentaje indica cuánto tiempo pudo avanzar dicho flujo.
`es` cuenta resultados atendidos por ISR, no solo transferencias terminadas.
`lat_sum` acumula el tiempo desde cada finalización hasta aceptar su IRQ.

El CSV contiene registros del PIC, modo/PC/IF, etapa y vector, solicitudes en
servicio, tiempos restantes, colas, finalizaciones pendientes y métricas.
No contiene columnas de procesos ni planificación. El formato cambia respecto
de la versión anterior y los consumidores deben usar los nuevos encabezados.

## Pruebas

`make test` ejecuta la suite de E/S y CPU Core. Las pruebas del motor cubren:

- Ausencia de IRQ cuando no hay E/S.
- Una finalización atendida por ISR, identidad y FIFO.
- Conservación de múltiples IRQ con IMR y con IF deshabilitado.
- Prioridad, vectores y latencia desde la finalización.
- Restauración exacta del contexto y reanudación de instrucciones.
- Avance de dispositivos durante ISR y anidamiento con/sin EOI temprano.
- Capacidad de las colas, entradas inválidas y recuperación de capacidad.
- 3000 ciclos para cada combinación de las tres variantes, seguidos por el
  vaciado de solicitudes; se comprueba conservación y progreso de ambos dispositivos.

Las prioridades fijas no garantizan equidad bajo una carga arbitraria.
La prueba prolongada verifica el progreso de la carga de demostración.

## Organización e integración

GUI y CLI usan el mismo motor en `src/nucleo/simulador.c`.
Los módulos académicos `cpu_core.c`, `context_switch.c` y
`vector_interruptions.c` se mantienen como implementaciones independientes.
El contexto académico admite un solo marco; el motor necesita una pila para
las ISR anidadas. Unificar esas APIs no forma parte de esta limpieza de alcance.
