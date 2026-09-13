/*
Aquí se ejecuta el cambio de contexto, utilizando el archivo context.h.
Es donde se recupera el estado anterior a la interrupción, que quedó
guardado en los registros.
*/
#include <stdlib.h>      
#include "contexto.h"

// Puntero que apunta al espacio de memoria donde se almacenan los contextos
Contexto *contextos = NULL;

// Cantidad de contextos que actualmente estamos utilizando
int cantidad_contextos = 0;

// Cantidad de contextos que podemos almacenar con la reserva inicial
int capacidad_contextos = 0;

// Reservamos inicialmente espacio para 3 procesos
int inicializar_contextos(void){
    capacidad_contextos = 3;

    // Reservamos memoria para 3 estructuras Contexto
    contextos = malloc(capacidad_contextos * sizeof(Contexto));

    // Comprobamos si se pudo reservar la memoria
    if (contextos == NULL){
        // Si malloc falla, dejamos los valores en cero
        capacidad_contextos = 0;
        return 0;
    }

    // Iniciamos la cantidad de contextos
    cantidad_contextos = 0;

    return 1;
}

// Funcion para buscar un contexto y evitar repeticiones de contexto para un mismo proceso
int buscar_contexto(int id_proceso){
    int i;

    // Recorremos solamente los espacios que estamos utilizando
    for (i = 0; i < cantidad_contextos; i++){
        // Comparamos el ID del proceso
        if (contextos[i].id_proceso == id_proceso){
            // Retornamos la posición donde se encontró
            return i;
        }
    }
    return -1;
}

int guardar_contexto(int id_proceso, Registro registro, EstadoProceso estado){
    int posicion;

    // Verificamos si el proceso ya tiene un contexto guardado
    posicion = buscar_contexto(id_proceso);

    // Si ya existe
    if (posicion != -1){
        // Actualizamos el registro del proceso
        contextos[posicion].registro = registro;

        // Actualizamos su estado
        contextos[posicion].estado = estado;

        return 1;
    }


    
    // Si es nuevo
    // Comprobamos si ya utilizamos todos los espacios
    if (cantidad_contextos >= capacidad_contextos)
    {
        Contexto *temporal;

        // Duplicamos la capacidad actual
        int nueva_capacidad = capacidad_contextos * 2;

        // Ampliamos el bloque de memoria
        temporal = realloc(contextos, nueva_capacidad * sizeof(Contexto));

        // Si realloc falla, no podemos guardar el contexto
        if (temporal == NULL){
            return 0;
        }

        // Actualizamos el puntero con el nuevo espacio
        contextos = temporal;

        // Guardamos la nueva capacidad
        capacidad_contextos = nueva_capacidad;
    }


    // Guardamos el nuevo contexto

    // Guardamos el ID del proceso
    contextos[cantidad_contextos].id_proceso = id_proceso;

    // Guardamos una copia de todos sus registros:
    contextos[cantidad_contextos].registro = registro;

    // Guardamos el estado del proceso
    contextos[cantidad_contextos].estado = estado;

    // Aumentamos la cantidad de contextos utilizados
    cantidad_contextos++;

    return 1;
}

// Funcion para recuperar el registro (donde se encuentra el estado) guardado para el proceso
int restaurar_contexto(int id_proceso, Registro *registro){
    int posicion;

    // Buscamos el contexto del proceso
    posicion = buscar_contexto(id_proceso);

    // Si el proceso no existe, no podemos restaurarlo
    if (posicion == -1){
        return 0;
    }

    // Copiamos el registro guardado al registro actual
    *registro = contextos[posicion].registro;

    return 1;
}

// Funcion para liberar la memoria
void liberar_contextos(void)
{
    // Liberamos la memoria reservada 
    free(contextos);

    // Apuntamos hacia una direccion de memoria no liberada
    contextos = NULL;

    // Reiniciamos los contadores
    cantidad_contextos = 0;
    capacidad_contextos = 0;
}