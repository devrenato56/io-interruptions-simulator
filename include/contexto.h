/*
Define la estructura que conserva el estado del CPU interrumpido por una operación de E/S.
*/

// Definimos el contexto
#ifndef CONTEXTO_H
#define CONTEXTO_H

// Utilizaremos los registros, son parte fundamental de contexto
#include "registro.h"

// Definimos la estructura que permitirá recuperar el estado anterior a la interrupción de E/S
typedef struct {

    Registro registro; // Copia de los registros del CPU antes de atender la E/S

} Contexto;

#endif
