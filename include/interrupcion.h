/*
Archivo que define un struct con los conceptos fundamentales de una interrupcion.
Aquí es donde el número que se almacena en la línea de bus (vector de interrupción)
se materializa en memoria, y también se le asigna una prioridad a la interrupción.
*/

// Definimos el header de la interrupción
#ifndef INTERRUPCION_H
#define INTERRUPCION_H

// Definimos los tipos de interrupción que tendremos en nuestro sistema
typedef enum {

    INT_HARDWARE, // Periféricos, mouse, teclado, etc
    INT_SOFTWARE // Syscall, divisón entre cero, etc

} TipoInterrupcion;

// Definimos la estructura que tendrán las interrupciones con sus atributos
typedef struct {

    int numero; // Número de interrupción en la cola
    int prioridad; // Qué tan importante es la interrupción del 0 (muy urgente) al 5 (menos urgente)
    TipoInterrupcion tipo;

} Interrupcion;

#endif