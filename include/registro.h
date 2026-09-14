/*
Archivo que define el estado observable del CPU. Este se guarda en los registros,
pequeños elementos que se encuentran exactamente en el procesador. Aquí se guardan cosas como el contador del programa,
las flags, entre otros. Este estado permite continuar la ejecución después de una interrupción de E/S.
*/

// Comenzamos definiendo el registro y los atributos que contendrá su struct
#ifndef REGISTRO_H
#define REGISTRO_H

typedef struct {

    unsigned int PC; // Program Counter, indica la próxima instrucción que el CPU ejecutará
    int registros_generales[8]; // Registros simples, array de 8, permitirán instrucciones como SUMA y CARGA
    unsigned char flags; // Condiciones del último resultado (cero, acarreo, error, etc)

} Registro;

#endif
