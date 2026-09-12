/*
Define la estructura inicial para poder guardar el estado de un proceso.
*/

// Definimos el contexto
#ifndef CONTEXTO_H
#define CONTEXTO_H

// Utilizaremos los registros, son parte fundamental de contexto
#include "registro.h"

// Definimos las constantes que representaran el estado de un proceso y que cambiarán según requiera
typedef enum {

    PROCESO_LISTO,
    PROCESO_EJECUTANDO,
    PROCESO_BLOQUEADO

} EstadoProceso;


// Definimos la estructura: cada proceso que se guarde en contexto, deberá tener id, un registro al que
// pertenece y el estado en el que se encontró, para regresar exactamente ahí
typedef struct {

    int id_proceso; // Identificador
    Registro registro; // Lugar donde se almacenará
    EstadoProceso estado; // Estado del proceso en el que se encontró

} Contexto;

#endif