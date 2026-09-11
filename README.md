# Simulador de interrupciones

Simulador educativo en C que modela el mecanismo de interrupciones de hardware y software en un sistema operativo: dispositivos de E/S, controlador de interrupciones (PIC), vector de interrupciones (IVT), cambio de contexto y un scheduler básico.

Proyecto desarrollado como parte del curso de Sistemas Operativos, con el objetivo de traducir a código los conceptos teóricos de interrupciones precisas/imprecisas, multiprogramación y tiempo compartido.

## Stack tecnológico

| Componente | Herramienta |
|---|---|
| Lenguaje | C (estándar C17) |
| Compilador | GCC 16.2.0 (MSYS2 / MinGW-w64 UCRT64) |
| Build system | Makefile (GNU Make) |
| Editor | Visual Studio Code |
| Extensiones | C/C++ (Microsoft, `ms-vscode.cpptools`) |
| Debugger | GDB (incluido en MSYS2) |
| Control de versiones | Git + GitHub |
| Estrategia de ramas | `feature/<nombre-fase>`, integración vía Pull Request a `main` |

### Por qué este stack

- **C puro, sin dependencias externas**: cualquier integrante del equipo puede compilar sin instalar librerías adicionales, solo el compilador base.
- **MSYS2/MinGW-w64 sobre Windows**: da acceso a GCC real (no MSVC), consistente con el estándar POSIX/GNU que se usa en la mayoría de material académico de sistemas operativos.
- **Sin frameworks ni librerías gráficas**: el simulador es una aplicación de consola; toda la complejidad está en la lógica de interrupciones, no en la interfaz.

## Requisitos previos

- GCC 16.2.0 o superior (via [MSYS2](https://www.msys2.org/))
- GDB (incluido con MSYS2)
- Make

Verificar instalación:
```bash
gcc --version
gdb --version
make --version
```

## Estructura del proyecto

```
simulador-interrupciones/
├── README.md
├── CONTEXT.md
├── .gitignore
├── Makefile
│
├── include/                # Headers compartidos (contratos entre fases)
│   ├── cpu.h
│   ├── registro.h
│   ├── interrupcion.h
│   └── contexto.h
│
├── src/
│   ├── main.c               # Orquesta la simulación completa
│   ├── cpu/
│   │   └── cpu_core.c        # Ciclo fetch-decode-execute
│   ├── contexto/
│   │   └── context_switch.c  # Guardar/restaurar contexto
│   ├── ivt/
│   │   └── vector_interrupciones.c  # Tabla de vectores de interrupción
│   ├── fuentes/
│   │   ├── timer.c            # Interrupción de temporizador
│   │   ├── teclado.c          # Interrupción de E/S
│   │   └── excepciones.c      # Excepciones de software
│   └── controlador/
│       ├── pic.c               # Controlador de interrupciones (arbitraje, prioridades)
│       └── scheduler.c         # Planificador de procesos
│
├── tests/
│   └── test_integracion.c
│
└── docs/
    └── decisiones_arquitectura.md
```

## Compilación y ejecución

```bash
make            # compila el proyecto
./main.exe      # ejecuta el simulador (Windows)
make clean      # limpia binarios generados
```

## Equipo y responsabilidades

Ver [CONTEXT.md](./CONTEXT.md) para el plan de trabajo completo, fases, y el flujo de funcionamiento del sistema.

## Licencia

Proyecto académico, uso educativo.
