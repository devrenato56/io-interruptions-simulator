/*
Declaración del dispositivo de E/S teclado simulado.
El teclado genera una solicitud de interrupción identificada
mediante un número de vector.
*/

#ifndef TECLADO_H
#define TECLADO_H

#include "interrupcion.h"

// Número de vector utilizado por el teclado simulado.
#define VECTOR_TECLADO 1

// Genera una solicitud de interrupción producida por el teclado.
Interrupcion teclado_generar_interrupcion(void);

#endif