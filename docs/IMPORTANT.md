# Proyecto SO - Simulador de interrupciones de E/S

## Alcance obligatorio

El proyecto simula únicamente interrupciones producidas por disco y teclado.
Se admiten prioridades, máscaras, colas y anidamiento de E/S.
No se deben añadir interrupciones de software, temporizador, excepciones,
scheduler ni cambio entre procesos.

En la línea por fases están avanzadas la Fase 1 (contratos) y la Fase 2 (CPU
Core). La demostración de E/S funciona en el motor independiente de
`src/nucleo/`; consulta `MANUAL.md` para compilar la consola y la GUI con raylib.

## Compilador GCC

El proyecto usa C17 y GCC. En Windows se recomienda instalar GCC mediante [MSYS2](https://www.msys2.org/) con el entorno UCRT64.

Después de instalar MSYS2, abre su terminal y actualiza los paquetes:

```bash
pacman -Syu
```

Si la terminal solicita cerrarse, vuelve a abrirla, repite la actualización e instala GCC:

```bash
pacman -Syu
pacman -S mingw-w64-ucrt-x86_64-gcc
```

Agrega esta ruta al `PATH` de Windows si el comando `gcc` no está disponible:

```text
C:\msys64\ucrt64\bin
```

Comprueba la instalación desde PowerShell o Command Prompt:

```powershell
gcc --version
where.exe gcc
```

Si existe otra instalación de GCC, la ruta de UCRT64 debe aparecer primero en `PATH` para que todo el equipo utilice el mismo compilador.

## Editor

Visual Studio Code con la extensión C/C++ de Microsoft permite completar código, detectar errores y depurar. También puede usarse cualquier editor que respete la configuración y los comandos del proyecto.

## Prueba disponible

Desde la raíz del repositorio, compila la prueba de las fases 1 y 2:

```powershell
gcc -std=c17 -Wall -Wextra -Iinclude tests/test_integration.c src/cpu/cpu_core.c -o test_cpu.exe
.\test_cpu.exe
```

Esta prueba valida el ciclo básico del CPU. `make test` ejecuta además
`tests/test_interrupciones.c`, que verifica el motor completo de E/S.
