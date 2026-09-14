/*
Declaración de la estructura del CPU y de sus funciones públicas.
Estas funciones permiten ejecutar instrucciones y comprobar solicitudes de E/S.
*/

// Definimos el módulo completo de CPU
#ifndef CPU_H
#define CPU_H

// Utilizaremos los dos módulos anteriores de registros e interrupciones
#include "registro.h"
#include "interrupcion.h"

// Definimos la estructura general del estado del CPU
typedef struct {

    int en_ejecucion; // Indica si el CPU debe continuar ejecutando instrucciones
    Registro registro; // Estado actual de los registros del CPU

} CPU;

// Función de inicialización del cpu
void cpu_inicializar(CPU* cpu);

// Función de ejecución del fetch -> decode -> execute
void cpu_ejecutar_ciclo(CPU* cpu);

// Función que identifica cuándo hay una interrupción de E/S en espera
int cpu_hay_interrupcion_pendiente(CPU* cpu);

// Función que atiende una interrupción de E/S detectada por el CPU
void cpu_atender_interrupcion(CPU* cpu, Interrupcion* interrupcion);

#endif
