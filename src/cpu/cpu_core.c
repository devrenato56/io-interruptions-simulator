/*
Ciclo fetch->decode->execute. Después de cada instrucción se comprueba
si un dispositivo de E/S mantiene una solicitud de interrupción pendiente.
*/

// Utilizamos el módulo principal que hemos creado hace un rato (header)
#include "cpu.h"
#include "pic.h" // Se incluye el controlador PIC

// Definimos los tipos de instrucciones que utilizaremos
typedef enum {

    INST_NOP, // Le dice al CPU que no haga nada
    INST_SUMA, // Toma un registro de cierta posicion y solicita que se le sume X
    INST_SALTO, // Pasa de un registro a otro
    INST_HALT // Le dice al CPU que pare

} TipoInstruccion;


// Asimismo, definimos de qué se compone una instrucción
typedef struct {

    TipoInstruccion tipo;
    int operando1;
    int operando2;

} Instruccion;

// Programa de simulación para que nuestro sistema tenga cierta memoria de la cual obtener información
// Caso contrario, como nuestra memoria está vacía, no podemos obtener nada
static Instruccion programa[] = {
    {INST_NOP, 0, 0}, // Se le colocan estos números siguiendo el patrón de la struct
    {INST_SUMA, 0, 5}, // Ese 5 es la cantidad que se le suma al registro de posición 0
    {INST_SALTO, 4, 0}, // Ese 4 pide un cambio de registro
    {INST_SUMA, 0, 99}, // Ese 99 es la cantidad que se le suma al registro 0
    {INST_HALT, 0, 0} // CPU para, ni halt ni nop tienen operandos
};

// Representamos temporalmente la señal pendiente de un dispositivo de E/S
// static int interrupcion_es_pendiente = 0;

// Utilizaremos el tamaño de este mini programa para validar el contador de programa
// Es importante porque evita acceder a una posición que no pertenece al arreglo
static const unsigned int tam_prog = sizeof(programa) / sizeof(programa[0]);

void cpu_inicializar(CPU* cpu) {
    /*
    Función de inicialización del CPU. Es la que le da el inicio al ciclo
    fetch -> decode -> execute. En este archivo verificamos si el cpu
    no es NULL y asignamos valores iniciales con punteros.
    */

    // Comprobamos si existe
    if(!cpu) {
        return;
    }

    // Asignamos valores, el en_ejecucion siempre en 1 para que dé a entender que ya se está usando el cpu
    cpu->en_ejecucion = 1;
    cpu->registro.PC = 0;
    cpu->registro.flags = 0;

    // Llenamos todos los registros con ceros
    for (int i = 0; i < 8; i++) {

        cpu->registro.registros_generales[i] = 0;

    }

}

// Función propia del archivo, no es pública para todos los demás archivos.
// Recupera la información requerida del programa creado.
static Instruccion cpu_fetch(CPU* cpu) {
    
    // Comprobamos que el CPU exista; de lo contrario devolvemos una instrucción de detención
    if(!cpu) {
        
        Instruccion halt = {INST_HALT, 0, 0};
        return halt;

    }

    // Comprobamos que el contador de programa permanezca dentro del programa simulado
    if(cpu->registro.PC >= tam_prog) {

        Instruccion halt = {INST_HALT, 0, 0};
        cpu->en_ejecucion = 0;
        return halt;

    }

    // Devuelve una instruccion
    return programa[cpu->registro.PC];

}

static void cpu_execute(CPU* cpu, Instruccion instruccion) {
    
    if(!cpu) {

        return;

    }

    // Ejecutamos la operación indicada por la instrucción obtenida durante el fetch
    switch (instruccion.tipo) {

        case INST_NOP:

            // Como en NOP no hace nada, pasa al siguiente
            cpu->registro.PC++;
            break;

        case INST_SUMA:

            // Comprobamos si el operando cabe en el índice de los registros
            if(instruccion.operando1 < 0 || instruccion.operando1 >= 8) {
                cpu->en_ejecucion = 0;
                return;
            }

            // Aplicamos la operacion en los registros y pasamos al siguiente
            cpu->registro.registros_generales[instruccion.operando1] += instruccion.operando2;
            cpu->registro.PC++;

            break;
        
        case INST_SALTO:

            // Lo mismo que en SUMA, solo que con salto
            if(instruccion.operando1 < 0 || (unsigned int)instruccion.operando1 >= tam_prog) {
                cpu->en_ejecucion = 0;
                return;
            }
            
            // Actualizamos el contador de programa con el destino del salto
            cpu->registro.PC = instruccion.operando1;
            break;
        
        case INST_HALT:
            
            // Detenemos porque eso hace halt
            cpu->en_ejecucion = 0;
            break;

    }

}

void cpu_ejecutar_ciclo(CPU* cpu) {
    // Función de ejecución completa del fetch->decode->execute.
    if(!cpu) {
        return;
    }

    if(cpu->en_ejecucion == 0) {
        return;
    }

    Instruccion instruccion = cpu_fetch(cpu);
    cpu_execute(cpu, instruccion);

    if(cpu_hay_interrupcion_pendiente(cpu) == 1) {

        // Creamos una interrupción de E/S identificada por su número de vector
        Interrupcion interrupcion = pic_obtener_siguiente();
        
        // Verificamos que sea una interrupción válida (número distinto de -1)
        if(interrupcion.numero != -1){
            cpu_atender_interrupcion(cpu, &interrupcion);
        }
        

    }

}

int cpu_hay_interrupcion_pendiente(CPU* cpu) {
    // Función que consulta temporalmente una señal de E/S pendiente
    if(!cpu) {
        return 0;
    }

    return pic_hay_pendientes();
}

void cpu_atender_interrupcion(CPU* cpu, Interrupcion* interrupcion) {
    if(!cpu || !interrupcion) {
        return;
    }

    // Limpiamos la señal de E/S después de atenderla
    //interrupcion_es_pendiente = 0;
}
