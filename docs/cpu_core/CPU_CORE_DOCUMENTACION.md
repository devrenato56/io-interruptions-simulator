# Documentacion - Fase 2: CPU Core

Este archivo explica que se hizo en la Fase 2 del proyecto, que corresponde al modulo de CPU Core.

La idea de esta fase es simular, de manera sencilla, como un CPU ejecuta instrucciones. No estamos creando
un CPU real ni una memoria real, sino una representacion pequena para entender el ciclo principal:

```text
fetch -> decode -> execute
```

Asimismo, se agrego un chequeo simple de interrupciones pendientes. Este chequeo todavia no representa al
PIC final del proyecto. Es solamente un stub temporal para que el CPU ya tenga el punto donde mas adelante
se conectara el controlador real de interrupciones.

## Archivos trabajados

Los archivos principales de esta fase fueron:

```text
include/registro.h
include/contexto.h
include/interrupcion.h
include/cpu.h
src/cpu/cpu_core.c
tests/test_integration.c
```

Los headers de `include/` funcionan como contratos. Es decir, son la forma en la que otros modulos del
proyecto saben que estructuras y funciones existen.

## Headers base

Primero se definieron las estructuras basicas que necesitaba el CPU Core.

En `registro.h` se definio `Registro`, que guarda el estado interno minimo del CPU:

```text
PC
registros_generales
flags
```

El `PC` es el Program Counter. Sirve para saber que instruccion toca ejecutar.

Los `registros_generales` son espacios pequenos donde el CPU guarda valores temporales.

Las `flags` representan banderas del CPU. Por ahora estan simples, pero mas adelante pueden servir para
marcar resultados especiales.

En `contexto.h` se definio `Contexto`, que representa el estado guardado de un proceso. Esto sera importante
cuando se trabaje el cambio de contexto.

En `interrupcion.h` se definio `Interrupcion`, que guarda:

```text
numero
prioridad
tipo
```

El numero servira para identificar la interrupcion, la prioridad servira para que el PIC decida que atender
primero, y el tipo permite diferenciar entre interrupciones de hardware y software.

En `cpu.h` se declaro la estructura `CPU` y las funciones publicas del modulo.

## Programa simulado

Como todavia no existe una memoria completa, se creo un programa simulado dentro de `cpu_core.c`.

Este programa es un arreglo de instrucciones. Basicamente funciona como una memoria pequena de prueba.

Cada instruccion tiene:

```text
tipo
operando1
operando2
```

Por ejemplo, una instruccion de suma puede decir:

```text
sumar 5 al registro 0
```

Entonces el CPU usa el `PC` para saber que posicion del arreglo debe leer.

Si `PC = 0`, lee la instruccion 0.

Si `PC = 1`, lee la instruccion 1.

Y asi sucesivamente.

## Ciclo fetch-decode-execute

El ciclo principal se implemento en `cpu_ejecutar_ciclo`.

Primero se revisa que el CPU exista y que este en ejecucion. Si no existe o ya esta detenido, no se hace nada.

Luego ocurre el fetch:

```text
se lee la instruccion que esta en programa[PC]
```

Despues ocurre el execute:

```text
se ejecuta la instruccion segun su tipo
```

El decode en esta version es simple, porque el tipo de instruccion ya viene dentro del struct `Instruccion`.
Por eso se usa un `switch` para decidir que hacer.

Las instrucciones implementadas fueron:

```text
INST_NOP
INST_SUMA
INST_SALTO
INST_HALT
```

`INST_NOP` no hace nada, solo avanza el `PC`.

`INST_SUMA` suma un valor a uno de los registros generales.

`INST_SALTO` cambia directamente el `PC` hacia otra instruccion.

`INST_HALT` detiene el CPU.

## Manejo del PC

El `PC` es una de las partes mas importantes de esta fase.

En instrucciones normales, como `NOP` o `SUMA`, el `PC` aumenta en 1.

En una instruccion de salto, el `PC` no aumenta normalmente. En ese caso, toma el valor indicado por la
instruccion.

Esto permite simular que el CPU puede saltarse partes del programa.

Por ejemplo, en el programa de prueba hay una suma de 99 que no debe ejecutarse, porque antes ocurre un salto.
Si al final el registro queda en 5 y no en 104, significa que el salto funciono.

## Stub temporal de interrupciones

Tambien se agrego una bandera interna:

```text
interrupcion_pendiente_simulada
```

Esta bandera simula si existe una interrupcion pendiente.

Por ahora no es el PIC real. Solamente permite que el CPU tenga ya este flujo:

```text
terminar una instruccion
revisar si hay interrupcion pendiente
atenderla si existe
```

Esto es importante porque la interrupcion se revisa despues de ejecutar la instruccion actual. Asi se mantiene
la idea de interrupcion precisa: el CPU no queda a medias con una instruccion.

Cuando la Fase 6 implemente el PIC real, este stub deberia ser reemplazado por una consulta al modulo del
controlador de interrupciones.

## Pruebas internas

Se agrego un test en:

```text
tests/test_integration.c
```

Este test usa las funciones publicas del CPU Core. No llama directamente a `fetch` ni a `execute`, porque esas
funciones son internas del archivo `cpu_core.c`.

La prueba revisa:

```text
inicializacion del CPU
avance del PC con NOP
suma sobre el registro 0
salto hacia otra instruccion
detencion con HALT
```

Tambien revisa que el salto omita la instruccion que sumaba 99. Esto confirma que el `PC` se esta modificando
correctamente.

## Como compilar las pruebas

Desde la raiz del proyecto:

```bash
gcc -Wall -Wextra -Iinclude tests/test_integration.c src/cpu/cpu_core.c -o test_cpu.exe
```

Luego se ejecuta:

```bash
.\test_cpu.exe
```

Si estas parado dentro de la carpeta `tests`, el comando cambia:

```bash
gcc -Wall -Wextra -I../include test_integration.c ../src/cpu/cpu_core.c -o test_integration.exe
```

Y se ejecuta asi:

```bash
.\test_integration.exe
```

## Resultado esperado

Si todo esta bien, el test debe mostrar varios mensajes `[OK]` y terminar con:

```text
Todas las pruebas de CPU Core pasaron.
```

## Nota para integracion futura

El stub de interrupciones no debe tomarse como implementacion final del PIC.

La Fase 6 debera encargarse de la cola de interrupciones, prioridades, mascaras y arbitraje. Cuando esa fase
este lista, la funcion que hoy consulta la bandera simulada deberia pasar a consultar al PIC real.

La idea es no comentar codigo manualmente para integrar, sino reemplazar el stub por una implementacion real
manteniendo el contrato publico del CPU.
