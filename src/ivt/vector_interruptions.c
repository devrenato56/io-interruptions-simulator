/*
Esta tabla permite localizar la rutina de servicio asociada al número de
vector entregado por un dispositivo de E/S.
*/

#include <stddef.h>

#include "ivt.h"

// Definimos la cantidad máxima de vectores disponibles en la tabla
#define MAX_VECTORES 256

// Agrupamos los manejadores registrados por su número de vector
typedef struct {
    ManejadorInterrupcion tabla[MAX_VECTORES];
} IVT;

// Mantenemos la tabla privada para evitar modificaciones desde otros módulos
static IVT tabla_vectores;

// Inicializamos todos los vectores sin un manejador asociado
void ivt_inicializar(void) {
    for(int i = 0; i < MAX_VECTORES; i++) {
        tabla_vectores.tabla[i] = NULL;
    }
}

// Asociamos una rutina de servicio con un número de vector válido
int ivt_registrar_manejador(int numero_interrupcion, ManejadorInterrupcion manejador) {
    // Comprobamos el rango del vector y la existencia del manejador
    if(numero_interrupcion < 0 || numero_interrupcion >= MAX_VECTORES || !manejador) {
        return 0;
    }

    tabla_vectores.tabla[numero_interrupcion] = manejador;

    return 1;
}

// Localizamos y ejecutamos la rutina correspondiente a la interrupción de E/S
int ivt_atender(CPU* cpu, Interrupcion* interrupcion) {
    // Comprobamos que el CPU y la interrupción recibidos sean válidos
    if(!cpu || !interrupcion) {
        return 0;
    }

    int numero = interrupcion->numero;

    // Comprobamos que el vector pertenezca a la tabla y tenga una rutina registrada
    if(numero < 0 || numero >= MAX_VECTORES || !tabla_vectores.tabla[numero]) {
        return 0;
    }

    tabla_vectores.tabla[numero](cpu, interrupcion);

    return 1;
}
