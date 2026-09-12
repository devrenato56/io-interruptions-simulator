/*
Ciclo fetch->decode->execute. Aquí se agrega un bloque nuevo que es la
adición de un chequeador de interrupciones pendientes en cola.
*/

// Utilizamos el módulo principal que hemos creado hace un rato (header)
#include "cpu.h"

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

// Stub estático para simulación de pic
static int interrupcion_pendiente_simulada = 0;

// Utilizaremos el tamaño de este mini programa para poder calcular y reaizar las operaciones con los registros
// Es importante porque si se pasa del tamaño de indice del arreglo, el programa se muere
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

    // Llenamos todos los registgros con ceros
    for (int i = 0; i < 8; i++) {

        cpu->registro.registros_generales[i] = 0;

    }

}

// Función propia del archivo, no es pública para todos los demás archivos.
// Recupera la información requerida del programa creado.
static Instruccion cpu_fetch(CPU* cpu) {
    
    // Comprobación de que el CPU está, sino, devuelve una instrucción halt de detención de cpu
    if(!cpu) {
        
        Instruccion halt = {INST_HALT, 0, 0};
        return halt;

    }

    // Comprobamos si el tamaño del program counter es mayor al del programa, para que no afecte a los registros
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

    // Acá ejecutamos según el tipo de instrucción que vayamos a utilizar. Eso viene del fetch
    switch (instruccion.tipo) {

        case INST_NOP:

            // Como en NOP no hace nada, pasa al siguiente
            cpu->registro.PC++;
            break;

        case INST_SUMA:

            // Comprobamos si el operando cabe en el indice de los registros
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
            
            // Aquí obteneos el operando1 que resulta ser igual al Program Counter
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

        Interrupcion interrupcion = {1, 1, INT_HARDWARE};
        cpu_atender_interrupcion(cpu, &interrupcion);

    }

}

int cpu_hay_interrupcion_pendiente(CPU* cpu) {
    //Funión que simula un PIC
    if(!cpu) {
        return 0;
    }

    return interrupcion_pendiente_simulada;
}

void cpu_atender_interrupcion(CPU* cpu, Interrupcion* interrupcion) {
    if(!cpu || !interrupcion) {
        return;
    }

    interrupcion_pendiente_simulada = 0;
}