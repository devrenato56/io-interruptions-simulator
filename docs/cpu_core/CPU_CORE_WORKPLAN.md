# Plan de trabajo - Fase 2: CPU Core

Equipo: Renato y Leonel

Rama: `feature/cpu-core`

Depende de: Fase 1 (Arquitectura base)

Estado: avanzada

## Alcance

La Fase 2 implementa el ciclo básico del CPU y deja preparado el punto donde se consultará una solicitud de E/S. No implementa el dispositivo, el controlador, la IVT, la ISR ni el guardado de contexto.

## Subfase 1 - Usar los contratos base

- Usar `Registro` para PC, registros generales y flags.
- Usar `Interrupcion` con un número de vector de E/S.
- Usar `CPU` para el registro actual y el estado de ejecución.
- Mantener las interfaces independientes de temporizadores, excepciones, software y scheduler.

Contrato mínimo de interrupción:

```c
typedef struct {
    int numero; // Número de vector que identifica al dispositivo de E/S
} Interrupcion;
```

Operaciones públicas requeridas:

```c
void cpu_inicializar(CPU* cpu);
void cpu_ejecutar_ciclo(CPU* cpu);
int cpu_hay_interrupcion_pendiente(CPU* cpu);
void cpu_atender_interrupcion(CPU* cpu, Interrupcion* interrupcion);
```

## Subfase 2 - Fetch

- Inicializar PC, registros, flags y estado de ejecución.
- Obtener la instrucción situada en `programa[PC]`.
- Detener el CPU si el PC queda fuera del programa simulado.
- Mantener la representación de instrucciones privada al CPU Core.

## Subfase 3 - Decode y execute

- Interpretar el tipo mediante un `switch`.
- Ejecutar `NOP`, `SUMA`, `SALTO` y `HALT`.
- Validar el índice de registro y el destino del salto.
- Actualizar el PC una sola vez según la instrucción.

Estas instrucciones existen únicamente para mantener al CPU ocupado y observar dónde puede aparecer una E/S. No representan fuentes de interrupción.

## Subfase 4 - Comprobación de E/S pendiente

- Consultar una bandera temporal después de ejecutar cada instrucción.
- Representar la solicitud mediante el número de vector del dispositivo.
- Invocar el punto de atención cuando exista una solicitud.
- Limpiar la señal después de su atención temporal.

La bandera será reemplazada en la Fase 6 por el estado del controlador de E/S. No se añade una API para generar eventos porque esa responsabilidad pertenece al dispositivo y al controlador.

## Subfase 5 - Documentación y pruebas

- Documentar las operaciones públicas y los bloques que no sean evidentes.
- Compilar con C17, `-Wall` y `-Wextra`.
- Probar inicialización, NOP, SUMA, SALTO y HALT.
- Reservar la prueba de interrupción completa para la integración de las fases 3 a 7.

## Orden de trabajo

```text
Contratos base
      |
      v
Fetch -> Decode y execute -> Comprobación de E/S -> Pruebas
```

## Criterios de cierre

- El módulo compila sin depender de archivos de fases posteriores.
- Las pruebas del CPU Core terminan sin errores.
- El chequeo de la solicitud ocurre entre instrucciones.
- No hay referencias a temporizador, interrupciones de software, excepciones o scheduler.
