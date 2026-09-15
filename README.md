# Simulador de interrupciones de E/S

Simulador educativo en C17 de un flujo de CPU interrumpido por disco y teclado.
Conserva prioridades, máscaras, colas de solicitudes, vectores, ISR anidadas,
EOI y restauración del contexto. Solo las transferencias de E/S generan IRQ.

No incluye temporizador de interrupciones, quantum, planificación ni cambio
entre procesos. Los ciclos de simulación y los tiempos de servicio permiten
observar la E/S; no son fuentes independientes de interrupciones.

## Compilar y ejecutar

El núcleo y las pruebas requieren GCC y GNU Make. La interfaz gráfica requiere
además **raylib**. No se accede al hardware real.

```bash
make test
make cli
./simulador --ciclos 400 --anidar --salida trace.csv
make gui
./simulador_gui
```

`make` compila la GUI por defecto. En Windows los ejecutables llevan
`.exe`. Para compilar sin Make desde PowerShell:

```powershell
gcc -std=c17 -Wall -Wextra -Iinclude tests/test_interrupciones.c src/nucleo/simulador.c -o test_interrupciones.exe
.\test_interrupciones.exe
gcc -std=c17 -Wall -Wextra -Iinclude src/nucleo/main_sim.c src/nucleo/simulador.c -o simulador.exe
.\simulador.exe --ciclos 400 --anidar --salida trace.csv
```

## Organización

| Ruta | Responsabilidad |
|---|---|
| `include/config_sim.h` | Dispositivos, vectores, prioridades y capacidad de solicitudes |
| `include/simulador.h` | Estado y API del motor |
| `src/nucleo/simulador.c` | Flujo de CPU, solicitudes, dispositivos, PIC y atención de E/S |
| `src/nucleo/main_sim.c` | Consola y traza CSV |
| `src/ui/main_gui.c` | Interfaz raylib, osciloscopio, bitácora y métricas |
| `tests/test_interrupciones.c` | Regresiones del motor de E/S |
| `src/cpu/`, `src/contexto/`, `src/ivt/` | Módulos de las fases académicas, independientes del motor |
| `tests/test_integration.c` | Prueba del CPU Core |
| `src/controlador/pic.c`, `src/fuentes/teclado.c`, `src/main.c` | Archivos preliminares de la integración por fases |

Los módulos académicos de CPU, contexto e IVT se conservan por pertenecer a E/S.
`make test` también los compila y ejecuta la prueba de CPU Core. Su conexión
con el motor de `src/nucleo/` sigue siendo trabajo de integración separado.

## Documentación

- [Manual](docs/MANUAL.md)
- [Arquitectura del motor](docs/ARQUITECTURA.md)
- [Contexto y alcance](docs/CONTEXT.md)
- [Plan por fases](docs/WORKPLAN.md)
- [Documentación del CPU Core](docs/cpu_core/CPU_CORE_DOCUMENTACION.md)
- [Preparación del entorno](docs/IMPORTANT.md)

## Licencia

Proyecto académico de uso educativo.
