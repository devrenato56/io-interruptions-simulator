/*
Este es el controlar de interrupciones. Básicamente, es el encargado de actuar
bajo una lógica de arbitraje que maneja prioridades cuando varias interrupciones
llegan a la vez. Utiliza el enmascaramiento de interrupciones (las ignora).
*/

#include "pic.h"

// Variables de estado interno (estáticas para que no sean accesibles desde fuera)
static Interrupcion cola_pendientes[MAX_PENDIENTES];
static int cantidad_pendientes = 0;
static int mascara[MAX_VECTORES];

// Inicializa o reinicia el estado del PIC
void pic_inicializar(void) {
    cantidad_pendientes = 0;
    
    // Desenmascaramos (permitimos) todos los vectores por defecto
    for (int i = 0; i < MAX_VECTORES; i++) {
        mascara[i] = 0; // 0 = Permitido, 1 = Enmascarado (ignorado)
    }
}

// Registra una nueva interrupción en la cola de espera
int pic_registrar_solicitud(Interrupcion interrupcion) {
    // Si la cola está llena, perdemos la interrupción (Opción 1 del plan)
    if (cantidad_pendientes >= MAX_PENDIENTES) {
        return 0; // Fallo
    }
    
    // Guardamos la interrupción al final de la cola
    cola_pendientes[cantidad_pendientes] = interrupcion;
    cantidad_pendientes++;
    
    return 1; // Éxito
}

// Verifica si hay al menos una interrupción pendiente que no esté enmascarada
int pic_hay_pendientes(void) {
    for (int i = 0; i < cantidad_pendientes; i++) {
        int numero_vector = cola_pendientes[i].numero;
        // Si encontramos una que NO está enmascarada, significa que sí hay pendientes válidas
        if (mascara[numero_vector] == 0) {
            return 1;
        }
    }
    return 0; // No hay pendientes o todas están bloqueadas por la máscara
}

// Busca la interrupción de mayor prioridad que no esté enmascarada
Interrupcion pic_obtener_siguiente(void) {
    // Creamos una interrupción nula (vacía) para retornar en caso de error
    Interrupcion nula = {-1, -1, INT_HARDWARE};
    
    if (cantidad_pendientes == 0) {
        return nula;
    }
    
    int indice_mejor = -1;
    int mejor_prioridad = 999; // Usamos un número muy alto como peor prioridad base
    
    // Recorremos la cola buscando la interrupción más importante
    for (int i = 0; i < cantidad_pendientes; i++) {
        int numero_vector = cola_pendientes[i].numero;
        
        // Solo evaluamos las interrupciones que NO están enmascaradas
        if (mascara[numero_vector] == 0) {
            // Mientras MENOR sea el número, MAYOR es la prioridad (ej. 0 le gana a 5)
            if (cola_pendientes[i].prioridad < mejor_prioridad) {
                mejor_prioridad = cola_pendientes[i].prioridad;
                indice_mejor = i;
            }
        }
    }
    
    // Si no encontramos ninguna válida (todas estaban enmascaradas)
    if (indice_mejor == -1) {
        return nula;
    }
    
    // Guardamos la ganadora para retornarla
    Interrupcion ganadora = cola_pendientes[indice_mejor];
    
    // Reorganizamos la cola (movemos todos los elementos detrás del ganador un espacio hacia adelante)
    for (int i = indice_mejor; i < cantidad_pendientes - 1; i++) {
        cola_pendientes[i] = cola_pendientes[i + 1];
    }
    cantidad_pendientes--; // Reducimos el contador ya que sacamos una interrupción
    
    return ganadora;
}

// Bloquea (ignora) las interrupciones que lleguen con este número de vector
void pic_enmascarar(int numero) {
    if (numero >= 0 && numero < MAX_VECTORES) {
        mascara[numero] = 1; // 1 = Enmascarado
    }
}

// Permite nuevamente que las interrupciones con este número de vector sean procesadas
void pic_desenmascarar(int numero) {
    if (numero >= 0 && numero < MAX_VECTORES) {
        mascara[numero] = 0; // 0 = Permitido
    }
}