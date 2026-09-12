/*
Declaración de funciones públicas que invocan una llamada al CPU.
La utilizarán el resto de archivos.
*/

// Definimos el módulo completo de CPU
#ifndef CPU_H
#define CPU_H

// Utilizaremos los dos módulos anteriores de registros e interrupciones
#include "registro.h"
#include "interrupcion.h"

// Definimos la estructura general del estado del CPU
typedef struct {

    int en_ejecucion; // Sabremos si hay algún proceso en ejecucion o no
    Registro registro; // Registro en el que se guarda el estado

} CPU;

// Función de inicialización del cpu
void cpu_inicializar(CPU* cpu);

// Función de ejecución del fetch -> decode -> execute
void cpu_ejecutar_ciclo(CPU* cpu);

// Función que identifica cuándo hay una interrupción en espera
int cpu_hay_interrupcion_pendiente(CPU* cpu);

// Función que atiende interrupciones
void cpu_atender_interrupcion(CPU* cpu, Interrupcion* interrupcion);

#endif