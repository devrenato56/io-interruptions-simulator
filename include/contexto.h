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

// Función que prepara el almacenamiento del contexto interrumpido
int inicializar_contextos(void);

// Función que guarda una copia de los registros antes de atender la E/S
int guardar_contexto(const Registro* registro);

// Función que restaura los registros para continuar la ejecución interrumpida
int restaurar_contexto(Registro* registro);

// Función que reinicia el almacenamiento al finalizar la simulación
void liberar_contextos(void);

#endif
