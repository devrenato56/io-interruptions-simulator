/*
Esta será la tabla que nosotros utilizaremos para poder identificar
interrupciones. Recordemos que el número del dispositivo, al ser un
interruptor E/S, se guarda en una línea de BUS y para acceder a ella
utilizamos un PUNTERO que apunte a dicho número en memoria.
*/

#include <stdio.h>
#include "../../include/interrupcion.h"
#include "../../include/cpu.h"

// Definimos el tipo de función para los manejadores de interrupción (ISRs)
typedef void (*ManejadorInterrupcion)(CPU* cpu, Interrupcion* interrupcion);

#define MAX_VECTORES 256  //tamaño máximo de la tabla

typedef struct {
    ManejadorInterrupcion tabla[MAX_VECTORES];
} IVT;

// Instancia global de la IVT para que pueda ser usada en el simulador
IVT tabla_vectores;

void ivt_inicializar() {
    for (int i = 0; i < MAX_VECTORES; i++) {
        tabla_vectores.tabla[i] = NULL;
    }
    printf("IVT: Tabla de vectores inicializada correctamente.\n");
}

// Registra una función en un número de interrupción 
void ivt_registrar_manejador(int numero_interrupcion, ManejadorInterrupcion manejador) {
    if (numero_interrupcion>= 0 && numero_interrupcion < MAX_VECTORES) {
        tabla_vectores.tabla[numero_interrupcion] = manejador;
        printf("IVT: Manejador registrado para la interrupcion %d.\n", numero_interrupcion);
    } else {
        printf("IVT Error: Numero de interrupcion fuera de rango (%d).\n", numero_interrupcion);
    }
}

// ejecuta la atención de la interrupción
void ivt_atender(CPU* cpu, Interrupcion* interrupcion) {
    int num = interrupcion->numero;
    
    if (num >= 0 && num < MAX_VECTORES && tabla_vectores.tabla[num] != NULL) {
        tabla_vectores.tabla[num](cpu, interrupcion);
    } else {
        printf("IVT: Manejador no encontrado para int %d. Usando por defecto de CPU.\n", num);
        cpu_atender_interrupcion(cpu, interrupcion);
    }
}
