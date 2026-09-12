# Plan de trabajo — Fase 2: CPU Core

Equipo: Renato y Leonel
Rama: `feature/cpu-core`
Depende de: Fase 1 (Arquitectura base)

## Contexto

Los headers de la Fase 1 (`include/cpu.h`, `registro.h`, `interrupcion.h`, `contexto.h`) están creados pero vacíos. Como esta fase es la primera en necesitarlos con contenido real, la subfase 1 se encarga de definirlos con lo mínimo indispensable para que el CPU Core funcione. Estos mismos headers serán consumidos después por las fases 3, 4, 5 y 6 — cualquier cambio posterior a su forma debe avisarse al equipo.

---

## SUBFASE 1 (encargado: Renato) — Definir los headers base

-> Definir `registro.h`: struct con el estado del CPU (PC, registros de propósito general, flags)
-> Definir `contexto.h`: struct que envuelve el registro + metadatos del proceso (id, estado: listo/ejecutando/bloqueado)
-> Definir `interrupcion.h`: struct con número de interrupción, prioridad, y tipo (hardware/software)
-> Definir `cpu.h`: declarar las funciones públicas del CPU (`cpu_ejecutar_ciclo`, `cpu_atender_interrupcion`, `cpu_hay_interrupcion_pendiente`)
-> Compilar un `.c` de prueba vacío que solo incluya los 4 headers, para confirmar que no hay errores de sintaxis ni dependencias circulares entre ellos

### Contenido propuesto de los headers

**`include/registro.h`**
```c
#ifndef REGISTRO_H
#define REGISTRO_H

typedef struct {
    unsigned int PC;              // Contador de programa
    int registros_generales[8];   // Registros de propósito general
    unsigned char flags;          // Registro de banderas (zero, carry, etc.)
} Registro;

#endif
```

**`include/contexto.h`**
```c
#ifndef CONTEXTO_H
#define CONTEXTO_H

#include "registro.h"

typedef enum {
    PROCESO_LISTO,
    PROCESO_EJECUTANDO,
    PROCESO_BLOQUEADO
} EstadoProceso;

typedef struct {
    int id_proceso;
    Registro registro;
    EstadoProceso estado;
} Contexto;

#endif
```

**`include/interrupcion.h`**
```c
#ifndef INTERRUPCION_H
#define INTERRUPCION_H

typedef enum {
    INT_HARDWARE,
    INT_SOFTWARE
} TipoInterrupcion;

typedef struct {
    int numero;                 // índice en la IVT
    int prioridad;               // menor número = mayor prioridad
    TipoInterrupcion tipo;
} Interrupcion;

#endif
```

**`include/cpu.h`**
```c
#ifndef CPU_H
#define CPU_H

#include "registro.h"
#include "interrupcion.h"

typedef struct {
    Registro registro;
    int en_ejecucion; // 1 = corriendo, 0 = detenido
} CPU;

void cpu_inicializar(CPU* cpu);
void cpu_ejecutar_ciclo(CPU* cpu);
int  cpu_hay_interrupcion_pendiente(CPU* cpu);
void cpu_atender_interrupcion(CPU* cpu, Interrupcion* interrupcion);

#endif
```

---

## SUBFASE 2 (encargado: Leonel) — Ciclo Fetch

-> Implementar `cpu_inicializar()`: pone el CPU en un estado inicial válido (PC en 0, registros en 0, flags limpias)
-> Implementar la etapa de **fetch**: leer la "instrucción" en la posición actual del PC desde un arreglo simulado de instrucciones
-> Definir una representación mínima de instrucción (puede ser tan simple como un `enum` con 3-4 operaciones de prueba: `NOP`, `SUMA`, `SALTO`, `HALT`)
-> Escribir un test manual en `main.c` temporal que inicialice el CPU y haga fetch de 3-4 instrucciones, imprimiendo el PC en cada paso

## SUBFASE 3 (encargado: Renato) — Decode y Execute

-> Implementar la etapa de **decode**: interpretar la instrucción leída y determinar qué acción ejecutar
-> Implementar la etapa de **execute** para cada tipo de instrucción definida en la Subfase 2 (ej. `SUMA` modifica un registro, `SALTO` modifica el PC directamente)
-> Incrementar el PC correctamente después de cada instrucción (excepto en saltos, donde el PC ya fue modificado por la instrucción misma)
-> Unir fetch-decode-execute en una sola función `cpu_ejecutar_ciclo()` que haga las tres etapas en orden

## SUBFASE 4 (encargado: Leonel) — Chequeo de interrupciones pendientes

-> Implementar `cpu_hay_interrupcion_pendiente()`: por ahora, una función simple que consulta una variable/bandera global o una cola simulada (esto se conectará después con el PIC de la Fase 6)
-> Integrar el chequeo dentro de `cpu_ejecutar_ciclo()`: después de cada ciclo fetch-decode-execute, verificar si hay una interrupción pendiente
-> Si hay una interrupción pendiente, el ciclo debe detener el flujo normal y llamar a `cpu_atender_interrupcion()` (la implementación real de qué hace esta función se conecta con la Fase 3 de Context Switching y la Fase 4 de IVT — por ahora puede ser un stub que solo imprime "interrupción detectada")

## SUBFASE 5 (encargado: Renato) — Documentación y pruebas de integración interna

-> Documentar en comentarios el contrato de cada función pública de `cpu.h` (qué recibe, qué devuelve, qué efectos secundarios tiene)
-> Escribir 3-4 casos de prueba en `tests/` que cubran: ciclo normal sin interrupciones, ciclo con una interrupción simulada, instrucción de salto, instrucción HALT
-> Verificar que el módulo compila de forma aislada (`gcc -c src/cpu/cpu_core.c -Iinclude`) sin depender de que las demás fases ya existan
-> Preparar un mensaje/resumen para el equipo explicando la forma final de los 4 headers, ya que las fases 3, 4, 5 y 6 dependen de ellos

---

## Orden de trabajo recomendado

```
Subfase 1 (Renato) ──► bloquea a todo lo demás
        │
        ▼
Subfase 2 (Leonel) ──► Subfase 3 (Renato)
        │                     │
        └──────────┬──────────┘
                    ▼
          Subfase 4 (Leonel)
                    │
                    ▼
          Subfase 5 (Renato)
```

**Nota:** la Subfase 1 debe cerrarse primero, ya que tanto Renato como Leonel construyen sobre esos mismos headers. Una vez cerrada, Leonel puede avanzar con Subfase 2 mientras Renato empieza a preparar los casos de prueba de Subfase 5 en paralelo, aunque su ejecución completa dependa de que las subfases anteriores terminen.
