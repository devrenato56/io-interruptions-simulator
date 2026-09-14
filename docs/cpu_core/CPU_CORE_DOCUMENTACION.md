# Documentación - Fase 2: CPU Core

## Propósito

La Fase 2 implementa una representación sencilla de cómo el CPU ejecuta instrucciones y en qué momento comprueba una interrupción de E/S. No representa una CPU ni una memoria reales.

```text
fetch -> decode -> execute -> comprobar E/S pendiente
```

La comprobación ocurre al final del ciclo para evitar que una instrucción quede ejecutada parcialmente.

## Archivos de las fases 1 y 2

```text
include/registro.h
include/contexto.h
include/interrupcion.h
include/cpu.h
src/cpu/cpu_core.c
tests/test_integration.c
```

Los headers de `include/` son contratos compartidos. En particular:

- `Registro` guarda el `PC`, ocho registros generales y las flags.
- `Contexto` proporciona la estructura que una fase posterior usará para conservar el estado.
- `Interrupcion` guarda el número de vector del dispositivo de E/S.
- `CPU` guarda su estado de ejecución y expone las operaciones públicas.

No existe un campo para clasificar interrupciones de hardware o software. Todo objeto `Interrupcion` pertenece al único alcance admitido: E/S.

## Programa simulado

Mientras no exista un módulo de memoria, `cpu_core.c` contiene un arreglo privado de instrucciones:

```text
INST_NOP
INST_SUMA
INST_SALTO
INST_HALT
```

Cada instrucción tiene un tipo y dos operandos. El `PC` selecciona la posición que se obtiene durante `fetch`; el `switch` de `cpu_execute` realiza el decode y la ejecución.

- `INST_NOP` avanza el `PC`.
- `INST_SUMA` modifica un registro general y avanza el `PC`.
- `INST_SALTO` valida y reemplaza el `PC`.
- `INST_HALT` detiene el CPU.

Las validaciones de punteros, índices y límites impiden acceder fuera de los arreglos simulados.

## Punto de integración de E/S

La variable privada `interrupcion_es_pendiente` representa provisionalmente la señal que en la Fase 6 proporcionará el controlador. Su valor inicial es cero porque las fases 1 y 2 todavía no incluyen un dispositivo capaz de solicitar atención.

Al finalizar una instrucción, `cpu_ejecutar_ciclo` consulta `cpu_hay_interrupcion_pendiente`. Si existe una señal, crea una `Interrupcion` identificada por su número de vector y llama a `cpu_atender_interrupcion`.

Por ahora, la atención únicamente limpia la señal temporal. Las fases posteriores conectarán este punto con el guardado de contexto, la IVT, la ISR del teclado y el reconocimiento del controlador.

## Pruebas actuales

`tests/test_integration.c` comprueba el comportamiento que pertenece a la Fase 2:

- inicialización de PC, registros y estado del CPU;
- avance de `INST_NOP`;
- resultado de `INST_SUMA`;
- cambio de PC con `INST_SALTO`;
- detención con `INST_HALT`.

La prueba completa de una interrupción de E/S corresponde a la Fase 7, una vez que existan el dispositivo, el controlador, la IVT y la ISR.

## Compilación de la prueba

Desde la raíz del proyecto:

```powershell
gcc -std=c17 -Wall -Wextra -Iinclude tests/test_integration.c src/cpu/cpu_core.c -o test_cpu.exe
.\test_cpu.exe
```

El resultado correcto termina con:

```text
Todas las pruebas de CPU Core pasaron.
```

## Integración futura

La Fase 6 sustituirá la bandera privada por una consulta al controlador de E/S manteniendo las operaciones públicas del CPU. Este módulo no debe incorporar temporizador, excepciones, interrupciones de software ni lógica de planificación.
