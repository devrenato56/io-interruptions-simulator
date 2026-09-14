# Simulador de interrupciones de E/S

Simulador educativo en C del recorrido de una interrupción producida por un dispositivo de entrada/salida: solicitud del dispositivo, controlador simple, atención del CPU, guardado de estado, consulta de la IVT, ejecución de la ISR y retorno.

El proyecto no incluye interrupciones de software, temporizador, excepciones ni planificación de procesos. Actualmente solo se consideran avanzadas las fases 1 y 2: arquitectura base y CPU Core.

## Tecnología

| Componente | Herramienta |
|---|---|
| Lenguaje | C17 |
| Compilador | GCC / MinGW-w64 UCRT64 |
| Build system | GNU Make |
| Control de versiones | Git + GitHub |

No se utilizan frameworks ni dependencias externas. La simulación se ejecuta en consola y no interactúa con hardware real.

## Estructura actual

```text
io-interruptions-simulator/
|-- include/
|   |-- contexto.h
|   |-- cpu.h
|   |-- interrupcion.h
|   `-- registro.h
|-- src/
|   |-- contexto/context_switch.c
|   |-- controlador/pic.c
|   |-- cpu/cpu_core.c
|   |-- fuentes/teclado.c
|   |-- ivt/vector_interruptions.c
|   `-- main.c
|-- tests/test_integration.c
|-- docs/
|-- Makefile
`-- README.md
```

Los módulos de fases posteriores pueden existir como archivos preliminares, pero no se consideran implementados hasta completar su fase y sus pruebas.

## Compilar la prueba disponible

Desde la raíz del proyecto:

```powershell
gcc -std=c17 -Wall -Wextra -Iinclude tests/test_integration.c src/cpu/cpu_core.c -o test_cpu.exe
.\test_cpu.exe
```

La prueba actual valida inicialización, NOP, suma, salto y detención del CPU.

## Documentación

- [Contexto y flujo](docs/CONTEXT.md)
- [Plan de trabajo](docs/WORKPLAN.md)
- [Documentación del CPU Core](docs/cpu_core/CPU_CORE_DOCUMENTACION.md)
- [Plan de la Fase 2](docs/cpu_core/CPU_CORE_WORKPLAN.md)
- [Preparación del entorno](docs/IMPORTANT.md)

## Licencia

Proyecto académico de uso educativo.
