/*
Aquí se guarda el estado del CPU antes de atender una interrupción de E/S.
Después de ejecutar la rutina de atención, el mismo estado se restaura para
continuar desde la siguiente instrucción.
*/
#include "contexto.h"

// Conservamos un único contexto para reanudar la misma ejecución interrumpida
static Contexto contexto_interrumpido;

// Indicamos si existe un estado válido pendiente de restauración
static int contexto_guardado = 0;

// Preparamos el almacenamiento antes de iniciar la simulación
int inicializar_contextos(void) {
    contexto_interrumpido.registro = (Registro){0};
    contexto_guardado = 0;

    return 1;
}

// Guardamos una copia completa de los registros del CPU interrumpido
int guardar_contexto(const Registro* registro) {
    // Comprobamos el origen y evitamos sobrescribir un contexto pendiente
    if(!registro || contexto_guardado == 1) {
        return 0;
    }

    contexto_interrumpido.registro = *registro;
    contexto_guardado = 1;

    return 1;
}

// Restauramos los registros para reanudar la ejecución interrumpida
int restaurar_contexto(Registro* registro) {
    // Comprobamos el destino y la existencia de un contexto guardado
    if(!registro || contexto_guardado == 0) {
        return 0;
    }

    *registro = contexto_interrumpido.registro;
    contexto_guardado = 0;

    return 1;
}

// Reiniciamos el almacenamiento al finalizar la simulación
void liberar_contextos(void) {
    contexto_interrumpido.registro = (Registro){0};
    contexto_guardado = 0;
}
