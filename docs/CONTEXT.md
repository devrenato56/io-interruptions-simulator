# Contexto del proyecto

## Qué es esto

Un simulador en C que reproduce, a nivel educativo, el mecanismo de interrupciones de un sistema operativo real. No es un sistema operativo funcional ni interactúa con hardware real — es una simulación por software que modela el flujo completo que ocurre cuando un dispositivo (o el propio programa) necesita la atención del CPU.

El objetivo no es solo que el simulador "funcione", sino que el código refleje con la mayor fidelidad posible los conceptos teóricos vistos en clase: interrupciones de hardware vs. software, precisión de interrupciones, el controlador de interrupciones (PIC), el vector de interrupciones (IVT), el cambio de contexto, y el rol del scheduler en el tiempo compartido.

## Marco teórico que sustenta el diseño

El simulador está construido sobre dos ejes de clasificación de interrupciones, tratados como independientes entre sí:

**Según la fuente del evento:**
- Hardware: E/S (teclado en este proyecto), temporizador, fallos de hardware
- Software: trampas intencionales (syscalls) y excepciones no intencionales (división entre cero, fallo de página)

**Según la precisión del estado del CPU al momento de atender la interrupción** (Walker y Cragon, 1995):
- Precisas: existe una frontera nítida entre instrucciones completadas y no ejecutadas
- Imprecisas: el estado queda en un mosaico de instrucciones parcialmente ejecutadas

El simulador prioriza representar **interrupciones precisas**, ya que es el modelo necesario para poder reanudar un proceso interrumpido de forma correcta — algo indispensable en un simulador pedagógico donde se quiere mostrar el ciclo completo de guardar y restaurar contexto sin ambigüedad.

## Plan de trabajo y fases

| Fase | Equipo | Qué hará | Depende de | Rama Git |
|---|---|---|---|---|
| 1. Arquitectura base (interfaces/structs) | Persona 1 (solitario) | Definir estructuras compartidas: CPU, Registro, Interrupción, Contexto | Ninguna | `feature/arquitectura-base` |
| 2. CPU Core (fetch-decode-execute) | Persona 2 y 3 | Loop de ejecución de instrucciones, PC, chequeo de interrupciones pendientes | Fase 1 | `feature/cpu-core` |
| 3. Registros y Context Switching | Persona 4 y 5 | Guardar/restaurar contexto (registros, PC, flags) | Fase 1 | `feature/context-switching` |
| 4. Tabla de Vectores de Interrupción (IVT) | Persona 6 y 7 | Estructura IVT y registro de handlers | Fase 1 | `feature/ivt` |
| 5. Fuentes de interrupción (Timer, Teclado, Excepciones) | Persona 8 y 9 | Interrupciones de timer, teclado (I/O) y excepciones de software | Fase 3, Fase 4 | `feature/fuentes-interrupcion` |
| 6. Controlador (PIC) y Scheduler | Persona 10 y 11 | Cola de interrupciones, prioridades, máscara de interrupciones, scheduler básico | Fase 2, Fase 3, Fase 4 | `feature/pic-scheduler` |
| 7. Integración y Testing | Todos | Merge de ramas, pruebas de integración, logging final | Fases 2, 3, 4, 5, 6 | `feature/integracion-testing` |

**Nota sobre dependencias:** la Fase 1 es la única sin dependencias y bloquea directa o indirectamente a todas las demás. Para minimizar el cuello de botella, los headers de `include/` deben publicarse como contratos tempranos (aunque no estén 100% finalizados) para que las fases 2, 3 y 4 puedan comenzar a compilar contra ellos en paralelo.

## Workflow del sistema (flujo de ejecución)

Esta es la secuencia completa que el simulador reproduce cada vez que ocurre una interrupción, de principio a fin:

```
1. GENERACIÓN DEL EVENTO
   Un dispositivo (teclado) termina su operación, o el temporizador
   completa su cuenta, o el CPU detecta una condición de excepción.
        │
        ▼
2. NOTIFICACIÓN AL CONTROLADOR (PIC)
   El dispositivo/fuente impone una señal que el controlador de
   interrupciones recibe y evalúa.
        │
        ▼
3. ARBITRAJE
   El PIC decide si atiende la interrupción de inmediato o la
   posterga, según:
     - si hay otra interrupción en curso
     - la prioridad relativa del nuevo evento
     - si esa línea de interrupción está enmascarada
        │
        ▼
4. INTERRUPCIÓN AL CPU
   El PIC emite la señal final hacia el CPU, junto con el número
   que identifica la fuente del evento.
        │
        ▼
5. GUARDADO DE CONTEXTO
   El CPU detiene su ejecución normal. Antes de saltar al manejador,
   se guarda el contexto del proceso interrumpido (PC, registros,
   flags) usando las estructuras de contexto.h y registro.h.
        │
        ▼
6. CONSULTA AL VECTOR DE INTERRUPCIONES (IVT)
   El número de interrupción se usa como índice en la tabla IVT
   para obtener la dirección del manejador (ISR) correspondiente.
        │
        ▼
7. EJECUCIÓN DEL MANEJADOR (ISR)
   Se ejecuta la rutina específica para ese evento (ISR de teclado,
   ISR de timer, manejador de excepción).
        │
        ▼
8. DECISIÓN DEL SCHEDULER (si aplica)
   Si el evento fue el temporizador, o si el proceso interrumpido
   quedó bloqueado, el scheduler decide qué proceso ejecuta a
   continuación.
        │
        ▼
9. CAMBIO DE CONTEXTO (si el scheduler eligió otro proceso)
   Se restaura el contexto del proceso siguiente, sobrescribiendo
   los registros del CPU con los valores guardados previamente
   para ese proceso.
        │
        ▼
10. RECONOCIMIENTO AL CONTROLADOR
    El manejador de interrupciones le indica al PIC que ya se hizo
    cargo, liberando esa línea para futuras interrupciones.
        │
        ▼
11. RETORNO A EJECUCIÓN NORMAL
    El CPU retoma el ciclo fetch-decode-execute, ya sea del proceso
    original o del nuevo proceso elegido por el scheduler.
```

## Convenciones de trabajo

- Cada fase se desarrolla en su rama `feature/<nombre>` correspondiente, nunca directo sobre `main`.
- Los cambios se integran a `main` vía Pull Request, revisados por al menos otra persona del equipo.
- Cualquier cambio a los headers de `include/` (Fase 1) después de que otras fases ya dependan de ellos debe comunicarse al equipo antes de mergear, ya que rompe la compilación de las fases dependientes.
- La Fase 7 (integración) no comienza hasta que las fases 2, 3, 4, 5 y 6 tengan al menos una versión compilable en su rama.

## Referencias teóricas

- Tanenbaum, A. — *Sistemas Operativos Modernos*, 3ª ed., Capítulo 1 (sección 1.3.5) y Capítulo 5 (secciones 5.1.5, 5.2.3, 5.3.1).
- Wolf, G. et al. — *Fundamentos de Sistemas Operativos*, UNAM, 2015. Sección 2.2.2 "Interrupciones y excepciones".
- Walker, D. y Cragon, H. — *Interrupt Processing in Pipelined Processors*, 1995. Fuente de las cuatro propiedades formales de interrupción precisa.
