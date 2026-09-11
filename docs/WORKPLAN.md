# Plan de trabajo

Este documento detalla las fases de desarrollo del simulador de interrupciones, el equipo responsable de cada una, qué entrega cada fase, sus dependencias, y la rama Git correspondiente.

## Resumen de fases

| Fase | Equipo | Qué hará | Fase(s) limitante(s) | Rama Git |
|---|---|---|---|---|
| 1. Arquitectura base (interfaces/structs) | Persona 1 (solitario) | Definir estructuras compartidas: CPU, Registro, Interrupción, Contexto | Ninguna | `feature/arquitectura-base` |
| 2. CPU Core (fetch-decode-execute) | Persona 2 y 3 | Loop de ejecución de instrucciones, PC, chequeo de interrupciones pendientes | Fase 1 | `feature/cpu-core` |
| 3. Registros y Context Switching | Persona 4 y 5 | Guardar/restaurar contexto (registros, PC, flags) | Fase 1 | `feature/context-switching` |
| 4. Tabla de Vectores de Interrupción (IVT) | Persona 6 y 7 | Estructura IVT y registro de handlers | Fase 1 | `feature/ivt` |
| 5. Fuentes de interrupción (Timer, Teclado, Excepciones) | Persona 8 y 9 | Interrupciones de timer, teclado (I/O) y excepciones de software | Fase 3, Fase 4 | `feature/fuentes-interrupcion` |
| 6. Controlador (PIC) y Scheduler | Persona 10 y 11 | Cola de interrupciones, prioridades, máscara de interrupciones, scheduler básico | Fase 2, Fase 3, Fase 4 | `feature/pic-scheduler` |
| 7. Integración y Testing | Todos | Merge de ramas, pruebas de integración, logging final | Fases 2, 3, 4, 5, 6 | `feature/integracion-testing` |

## Detalle por fase

### Fase 1 — Arquitectura base (interfaces/structs)
- **Equipo:** Persona 1 (solitario)
- **Entregable:** Estructuras compartidas que usará todo el proyecto: CPU, Registro, Interrupción, Contexto.
- **Depende de:** Ninguna fase — es el punto de partida.
- **Rama:** `feature/arquitectura-base`
- **Nota:** Es la única fase sin dependencias y bloquea, directa o indirectamente, a todas las demás. Los headers deben publicarse como contrato temprano para que las fases 2, 3 y 4 puedan comenzar en paralelo sin esperar la versión final.

### Fase 2 — CPU Core (fetch-decode-execute)
- **Equipo:** Persona 2 y 3
- **Entregable:** Loop de ejecución de instrucciones, manejo del contador de programa (PC), chequeo de interrupciones pendientes en cada ciclo.
- **Depende de:** Fase 1
- **Rama:** `feature/cpu-core`

### Fase 3 — Registros y Context Switching
- **Equipo:** Persona 4 y 5
- **Entregable:** Lógica para guardar y restaurar el contexto de un proceso (registros, PC, flags).
- **Depende de:** Fase 1
- **Rama:** `feature/context-switching`

### Fase 4 — Tabla de Vectores de Interrupción (IVT)
- **Equipo:** Persona 6 y 7
- **Entregable:** Estructura de la IVT y el mecanismo de registro de handlers (manejadores de interrupción).
- **Depende de:** Fase 1
- **Rama:** `feature/ivt`

### Fase 5 — Fuentes de interrupción (Timer, Teclado, Excepciones)
- **Equipo:** Persona 8 y 9
- **Entregable:** Generación de interrupciones de temporizador, interrupciones de teclado (E/S), y excepciones de software.
- **Depende de:** Fase 3, Fase 4
- **Rama:** `feature/fuentes-interrupcion`

### Fase 6 — Controlador (PIC) y Scheduler
- **Equipo:** Persona 10 y 11
- **Entregable:** Cola de interrupciones, arbitraje por prioridades, máscara de interrupciones, y un scheduler básico.
- **Depende de:** Fase 2, Fase 3, Fase 4
- **Rama:** `feature/pic-scheduler`

### Fase 7 — Integración y Testing
- **Equipo:** Todos
- **Entregable:** Merge de todas las ramas, pruebas de integración, logging final del sistema.
- **Depende de:** Fases 2, 3, 4, 5, 6
- **Rama:** `feature/integracion-testing`
- **Nota:** No debe comenzar hasta que las fases 2, 3, 4, 5 y 6 tengan al menos una versión compilable en su respectiva rama.

## Diagrama de dependencias

```
                    Fase 1 (Arquitectura base)
                            │
          ┌─────────────────┼─────────────────┐
          ▼                 ▼                 ▼
      Fase 2            Fase 3            Fase 4
    (CPU Core)      (Context Switch)      (IVT)
          │                 │                 │
          │                 └────────┬────────┘
          │                          ▼
          │                      Fase 5
          │              (Fuentes de interrupción)
          │                          │
          └────────────┬─────────────┘
                        ▼
                    Fase 6
              (PIC y Scheduler)
                        │
                        ▼
                    Fase 7
              (Integración y Testing)
```

## Convenciones asociadas

- Cada fase se desarrolla en su rama `feature/<nombre>` correspondiente, nunca directo sobre `main`.
- Los cambios se integran a `main` vía Pull Request, revisados por al menos otra persona del equipo.
- Cualquier cambio a los headers de la Fase 1 después de que otras fases ya dependan de ellos debe comunicarse al equipo antes de mergear.
