# Contexto del proyecto

## Qué es esto

Este proyecto es un simulador educativo en C del mecanismo de una interrupción de entrada/salida (E/S) en un sistema operativo. No interactúa con hardware real: representa el recorrido de una solicitud desde un dispositivo simulado hasta el CPU y el regreso a la ejecución interrumpida.

El alcance se limita a interrupciones de E/S. No incluye interrupciones de temporizador, interrupciones de software, excepciones, llamadas al sistema, planificación de procesos ni cambio entre procesos.

## Objetivo

El simulador debe mostrar de forma sencilla este comportamiento:

1. El CPU ejecuta instrucciones mediante el ciclo `fetch -> decode -> execute`.
2. Un dispositivo de E/S simulado genera una solicitud.
3. Un controlador simple conserva la solicitud hasta que el CPU pueda atenderla.
4. El CPU termina la instrucción actual y detecta la interrupción pendiente.
5. Se guarda el estado mínimo del CPU.
6. La IVT localiza la rutina de servicio correspondiente al dispositivo.
7. La rutina atiende la E/S y reconoce la interrupción.
8. Se restaura el estado y el CPU continúa desde el punto correcto.

La interrupción se comprueba entre instrucciones. De este modo, el estado guardado contiene instrucciones completamente terminadas y un `PC` válido para reanudar la ejecución.

## Estado actual

En el desarrollo por fases se consideran avanzadas las fases 1 y 2:

- Fase 1: contratos base para `CPU`, `Registro`, `Interrupcion` y `Contexto`.
- Fase 2: ciclo básico del CPU y punto temporal de consulta de una E/S pendiente.

Los archivos presentes de fases posteriores no se consideran implementaciones terminadas hasta que su fase correspondiente sea desarrollada y probada.

El motor de `src/nucleo/` implementa por separado el flujo completo de E/S
para disco y teclado, con GUI y consola. Se permiten prioridades, máscaras,
colas y anidamiento por pertenecer a la atención de E/S. No incluye timer ni
planificación; su unificación con los módulos por fases sigue pendiente.

## Plan de trabajo

| Fase | Equipo | Entregable | Depende de | Rama Git |
|---|---|---|---|---|
| 1. Arquitectura base | Persona 1 | Estructuras mínimas de CPU, registro, interrupción de E/S y contexto | Ninguna | `feature/arquitectura-base` |
| 2. CPU Core | Persona 2 y 3 | Ciclo de instrucciones y comprobación de una E/S pendiente entre instrucciones | Fase 1 | `feature/cpu-core` |
| 3. Guardado de contexto | Persona 4 y 5 | Guardar y restaurar PC, registros y flags del flujo interrumpido | Fase 1 | `feature/context-switching` |
| 4. IVT e ISR de E/S | Persona 6 y 7 | Registrar y localizar el manejador del dispositivo por número de vector | Fase 1 | `feature/ivt` |
| 5. Dispositivo de E/S | Persona 8 y 9 | Simular el teclado y emitir su solicitud de interrupción | Fase 1 | `feature/fuente-es` |
| 6. Controlador de E/S | Persona 10 y 11 | Recibir, mantener, entregar y reconocer una solicitud pendiente | Fases 1, 2, 5 | `feature/pic-es` |
| 7. Integración y pruebas | Todos | Demostrar y probar el flujo completo de interrupción de E/S | Fases 2, 3, 4, 5, 6 | `feature/integracion-testing` |

## Flujo del sistema

```text
1. El teclado simulado completa una operación de E/S.
                         |
                         v
2. El dispositivo notifica al controlador de interrupciones.
                         |
                         v
3. El controlador conserva la solicitud como pendiente.
                         |
                         v
4. El CPU termina su instrucción actual y consulta el controlador.
                         |
                         v
5. El CPU guarda PC, registros generales y flags.
                         |
                         v
6. El número de vector permite localizar la ISR de teclado.
                         |
                         v
7. La ISR atiende la E/S y reconoce la solicitud.
                         |
                         v
8. El CPU restaura su estado y continúa la ejecución normal.
```

El teclado es el caso mínimo del desarrollo por fases. El motor incluye además
disco y conserva prioridades, máscaras y arbitraje entre ambas fuentes de E/S.
El número de vector identifica el dispositivo y su manejador.

## Convenciones de trabajo

- Cada fase se desarrolla en su rama `feature/<nombre>`.
- Los cambios se integran a `main` mediante Pull Request y revisión de otra persona.
- Los cambios a los headers de la Fase 1 deben comunicarse antes de integrar ramas dependientes.
- Ninguna fase debe añadir temporizador, excepciones, interrupciones de software o scheduler.
- La integración final comienza cuando las fases 2 a 6 tengan una versión compilable y probada.

## Referencias teóricas

- Tanenbaum, A. — *Sistemas Operativos Modernos*, 3.ª ed., secciones sobre E/S e interrupciones.
- Wolf, G. et al. — *Fundamentos de Sistemas Operativos*, UNAM, 2015, sección 2.2.2.
