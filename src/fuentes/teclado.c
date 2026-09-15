/*
Cuando se genera un evento de teclado, el dispositivo produce
una solicitud de interrupción.
*/

#include "teclado.h"

Interrupcion teclado_generar_interrupcion(void) {

    // Creamos la solicitud de interrupción del teclado.
    Interrupcion interrupcion;

    // Identificamos al teclado mediante su número de vector.
    interrupcion.numero = VECTOR_TECLADO;

    // Entregamos la solicitud para que posteriormente
    // pueda ser recibida por el controlador de interrupciones.
    return interrupcion;
}