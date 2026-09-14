/*
Definición de las funciones y constantes del Controlador Programable de Interrupciones (PIC).
Este módulo se encarga de recibir, filtrar (máscaras) y arbitrar (prioridades) 
las interrupciones antes de notificarlas al CPU.
*/

#ifndef PIC_H
#define PIC_H

#include "interrupcion.h"

// Capacidad máxima de la cola de espera del PIC
#define MAX_PENDIENTES 10
// Cantidad total de vectores de interrupción posibles
#define MAX_VECTORES 256

// Inicializa o reinicia el estado del PIC (limpia la cola y desenmascara todo)
void pic_inicializar(void);

// Registra una nueva interrupción en la cola de espera. Retorna 1 si tuvo éxito, 0 si la cola está llena.
int pic_registrar_solicitud(Interrupcion interrupcion);

// Verifica si hay al menos una interrupción pendiente que no esté enmascarada. Retorna 1 (sí) o 0 (no).
int pic_hay_pendientes(void);

// Busca la interrupción de mayor prioridad (número menor) que no esté enmascarada, la saca de la cola y la retorna.
Interrupcion pic_obtener_siguiente(void);

// Bloquea (ignora) las interrupciones que lleguen con este número de vector.
void pic_enmascarar(int numero);

// Permite nuevamente que las interrupciones con este número de vector sean procesadas.
void pic_desenmascarar(int numero);

#endif
