# Proyecto SO - Simulador de interruptores

Hola! Esto lo está escribiendo el desarrollador de la arquitectura del sistema y el que dejó los comentarios de los archivos.
Si te clonaste el repo y ya lo tienes en tu visual, o si simplemente lo estás leyendo desde GitHub, es importante que puedas
leer esto, ya que sino, el proyecto no corre.

# Compilador gcc

C corre con el compilador gcc. Por default, Windows no lo trae instalado. Entonces, es necesario que nosotros podamos instalarlo.
Tendrás que irte al siguiente link: https://www.msys2.org/ y seleccionar la opción que no tenga el ARM64.

Luego de eso, ejecutas el .exe del instalador, le das clic a continuar, y cuando finalice la instalación, se te abrirá una pequeña
terminal similar al git bash. En esta terminal, tipearás el siguiente comando:

´pacman -Sys´

pacman es un instalador de paquetes de Linux. Gracias a este, podrás descargar las dependencias necesarias. Una vez termine, el propio
proceso te dirá que para proceder con la instalación, se debera cerrar la terminal (aparecerá un (Y/n), tú púlsas Y). La terminal se cierra, y tú la vuelves a abrir buscando "MSYS2" en tu buscador de windows. Abres la terminal y vuelves a ejecutar:

´pacman -Sys´

Y cuando termine, ejecutas:

´pacman -S mingw-w64-ucrt-x86_64-gcc´

Así, estarás instalando el compilador gcc. Luego para comprobar que se instaló, abres terminal en powershell o Command Prompt, y ejecutas:

´gcc --version´

Si no te aparece nada, es porque falta agregarlo al PATH, entonces:
Vas al buscador de windows -> variables de entorno -> editar variables de entorno -> PATH -> Nuevo y añades la siguiente ruta:

C:\msys64\ucrt64\bin

Esta ruta que añadiste es la de instalación del compilador.
OJO: En caso por haber trabajado con C++ antes y eso, pulsarás la opción "Subir" para el path que añadiste recientemente, hasta que
quede encima por el PATH del compilador gcc antiguo que instalaste. Esto se hace porque si vuelves a ejecutar:

´where gcc´

Te saldrá primero la ruta del compilador antiguo, queriendo decir que lo está seleccionando antes a ese que al que tú descargaste.
Entonces, una vez teniendo encuenta eso y habiendo agregado al compilador al PATH, volvemos a ejecutar:

´gcc --version´
´where gcc´

Y debería estar todo OK.

# Extension de VS CODE

Recomiendo usar VS CODE para este proyecto y no antigravity porque VS CODE tiene una extensión desarrollada por Microsoft para C y C++.
Entonces, es más viable trabajar en VS CODE. De todas maneras, si desean usar Antigravity, pueden usarlo en CLI descargandolo en su
terminal.

Una vez hecho eso, pueden correr cualquier archivo de C en su editor de texto VS CODE. El comando para ejecutar el main es:

´gcc main.c -o main.exe´ (Compila el archivo)
´.\main.exe´ (Ejecuta el archivo SIEMPRE Y CUANDO estés parado en el directorio raíz, sino el comando se modifica)