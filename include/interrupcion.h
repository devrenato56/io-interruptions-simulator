/*
Archivo que define un struct con los datos de una interrupción de E/S.
Aquí es donde el número que se almacena en la línea de bus (vector de interrupción)
se materializa en memoria para identificar al dispositivo que solicita atención.
*/

// Definimos el header de la interrupción
#ifndef INTERRUPCION_H
#define INTERRUPCION_H

// Definimos la estructura que tendrán las interrupciones de E/S
typedef struct {

    int numero; // Número de vector que identifica al dispositivo de E/S

} Interrupcion;

#endif
