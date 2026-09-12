#include <stdio.h>

#include "cpu.h"

// Funcion auxiliar para comparar valores enteros durante las pruebas.
// Si el valor obtenido no coincide con el esperado, se reporta un error.
static int verificar_entero(const char* nombre, int obtenido, int esperado) {
    if (obtenido != esperado) {
        printf("[ERROR] %s: esperado=%d obtenido=%d\n", nombre, esperado, obtenido);
        return 1;
    }

    printf("[OK] %s\n", nombre);
    return 0;
}

int main(void) {
    // Contador general de errores encontrados durante la ejecucion del test.
    int errores = 0;

    // Creamos una instancia real de CPU para probar las funciones publicas.
    CPU cpu;

    // Inicializamos el CPU para dejar PC, registros, flags y estado en valores conocidos.
    cpu_inicializar(&cpu);

    // Verificamos el estado inicial que debe dejar cpu_inicializar.
    errores += verificar_entero("PC inicial", (int)cpu.registro.PC, 0);
    errores += verificar_entero("registro 0 inicial", cpu.registro.registros_generales[0], 0);
    errores += verificar_entero("CPU inicia en ejecucion", cpu.en_ejecucion, 1);

    // Primer ciclo: ejecuta INST_NOP, por lo tanto solo debe avanzar el PC.
    cpu_ejecutar_ciclo(&cpu);
    errores += verificar_entero("NOP incrementa PC", (int)cpu.registro.PC, 1);

    // Segundo ciclo: ejecuta INST_SUMA y suma 5 al registro general 0.
    cpu_ejecutar_ciclo(&cpu);
    errores += verificar_entero("SUMA incrementa PC", (int)cpu.registro.PC, 2);
    errores += verificar_entero("SUMA modifica registro 0", cpu.registro.registros_generales[0], 5);

    // Tercer ciclo: ejecuta INST_SALTO y mueve el PC directamente a la instruccion 4.
    cpu_ejecutar_ciclo(&cpu);
    errores += verificar_entero("SALTO modifica PC", (int)cpu.registro.PC, 4);

    // El salto debe omitir la instruccion que sumaba 99, por eso el registro sigue en 5.
    errores += verificar_entero("SALTO omite SUMA 99", cpu.registro.registros_generales[0], 5);

    // Cuarto ciclo: ejecuta INST_HALT y detiene el CPU.
    cpu_ejecutar_ciclo(&cpu);
    errores += verificar_entero("HALT detiene CPU", cpu.en_ejecucion, 0);
    errores += verificar_entero("registro 0 final", cpu.registro.registros_generales[0], 5);

    // Si no se acumulo ningun error, el modulo CPU Core paso las pruebas internas.
    if (errores == 0) {
        printf("Todas las pruebas de CPU Core pasaron.\n");
        return 0;
    }

    // Si hubo errores, se devuelve 1 para indicar fallo al sistema operativo.
    printf("Pruebas fallidas: %d\n", errores);
    return 1;
}
