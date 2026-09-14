# Plan de trabajo

Este plan organiza el desarrollo de un simulador dedicado únicamente a interrupciones de entrada/salida (E/S). Las fases 1 y 2 son las únicas avanzadas actualmente.

## Resumen de fases

| Fase | Equipo | Qué hará | Fase(s) limitante(s) | Rama Git | Estado |
|---|---|---|---|---|---|
| 1. Arquitectura base | Persona 1 (solitario) | Definir CPU, Registro, Interrupción de E/S y Contexto | Ninguna | `feature/arquitectura-base` | Avanzada |
| 2. CPU Core | Persona 2 y 3 | Ejecutar instrucciones y consultar una E/S pendiente entre ciclos | Fase 1 | `feature/cpu-core` | Avanzada |
| 3. Guardado de contexto | Persona 4 y 5 | Guardar y restaurar PC, registros y flags | Fase 1 | `feature/context-switching` | Pendiente |
| 4. IVT e ISR de E/S | Persona 6 y 7 | Asociar el número de vector con el manejador del dispositivo | Fase 1 | `feature/ivt` | Pendiente |
| 5. Dispositivo de E/S | Persona 8 y 9 | Simular el teclado y generar su solicitud | Fase 1 | `feature/fuente-es` | Pendiente |
| 6. Controlador de E/S | Persona 10 y 11 | Mantener, entregar y reconocer una solicitud pendiente | Fases 1, 2, 5 | `feature/pic-es` | Pendiente |
| 7. Integración y pruebas | Todos | Unir y verificar el flujo completo de E/S | Fases 2, 3, 4, 5, 6 | `feature/integracion-testing` | Pendiente |

## Detalle por fase

### Fase 1 - Arquitectura base

- Entregable: contratos mínimos compartidos en `include/`.
- `Registro` conserva PC, registros generales y flags.
- `Interrupcion` conserva el número de vector que identifica al dispositivo de E/S.
- `CPU` expone inicialización, ejecución y los puntos de consulta y atención.
- `Contexto` proporciona la base para conservar el estado interrumpido.

### Fase 2 - CPU Core

- Entregable: ciclo `fetch -> decode -> execute` con un programa simulado.
- El CPU valida su estado y el límite del programa.
- El chequeo de E/S ocurre después de terminar la instrucción actual.
- La señal pendiente es temporal y será sustituida por la consulta al controlador de la Fase 6.

### Fase 3 - Guardado de contexto

- Entregable: guardar y restaurar el estado del mismo flujo antes y después de la ISR.
- No selecciona procesos ni implementa planificación.
- Depende de la Fase 1.

### Fase 4 - IVT e ISR de E/S

- Entregable: una tabla mínima que relacione el número de vector del teclado con su rutina de servicio.
- La ISR procesa únicamente el evento del dispositivo de E/S.
- Depende de la Fase 1.

### Fase 5 - Dispositivo de E/S

- Entregable: un teclado simulado capaz de producir una solicitud de interrupción.
- No genera eventos periódicos ni condiciones internas del CPU.
- Depende de la Fase 1.

### Fase 6 - Controlador de E/S

- Entregable: recibir la solicitud del teclado, marcarla como pendiente, entregarla al CPU y limpiarla al recibir el reconocimiento.
- Al existir un único dispositivo, no incluye prioridades, máscaras, arbitraje ni scheduler.
- Depende de las fases 1, 2 y 5.

### Fase 7 - Integración y pruebas

- Entregable: ejecutar el CPU, generar una E/S, atenderla mediante su ISR y verificar que el CPU retoma el estado anterior.
- Debe incluir pruebas del flujo normal, solicitud pendiente, atención, reconocimiento y reanudación.
- Depende de las fases 2 a 6.

## Dependencias

```text
Fase 1 ──> Fase 2 ──────────────────────────────┐
   ├─────> Fase 3 ──────────────────────────────┤
   ├─────> Fase 4 ──────────────────────────────┤
   └─────> Fase 5 ──> Fase 6 ──────────────────┤
                                               v
                                      Fase 7: integración
```

## Fuera del alcance

- Interrupciones de temporizador.
- Interrupciones de software y llamadas al sistema.
- Excepciones del CPU.
- Scheduler, quantum y cambio entre procesos.
- Arbitraje entre múltiples fuentes, prioridades y máscaras.

## Convenciones asociadas

- Cada fase se desarrolla en su rama `feature/<nombre>` y se integra mediante Pull Request.
- Los contratos de la Fase 1 deben mantenerse pequeños y orientados al flujo de E/S.
- Una fase posterior no debe ampliar el alcance sin acuerdo previo del equipo.
