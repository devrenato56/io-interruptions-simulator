/*
Declaración de las funciones públicas de la Tabla de Vectores de Interrupción.
Estas funciones registran y localizan rutinas para dispositivos de E/S.
*/

// Definimos el módulo de la Tabla de Vectores de Interrupción
#ifndef IVT_H
#define IVT_H

// Utilizaremos las estructuras públicas del CPU y de la interrupción de E/S
#include "cpu.h"
#include "interrupcion.h"

// Definimos el tipo de función que tendrán las rutinas de servicio de E/S
typedef void (*ManejadorInterrupcion)(CPU* cpu, Interrupcion* interrupcion);

// Función que prepara la tabla sin manejadores registrados
void ivt_inicializar(void);

// Función que registra un manejador y devuelve uno cuando la operación tiene éxito
int ivt_registrar_manejador(int numero_interrupcion, ManejadorInterrupcion manejador);

// Función que ejecuta el manejador indicado y devuelve uno cuando logra atenderlo
int ivt_atender(CPU* cpu, Interrupcion* interrupcion);

#endif
