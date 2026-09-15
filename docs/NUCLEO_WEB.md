# Núcleo del simulador de E/S

El motor en C procede del prototipo web y ahora representa exclusivamente
interrupciones de E/S. Su comportamiento vigente se describe en
[ARQUITECTURA.md](ARQUITECTURA.md); las instrucciones de uso están en
[MANUAL.md](MANUAL.md).

Se conservan disco y teclado, sus colas FIFO, PIC con prioridades y máscaras,
conteo de eventos, vectores, contexto, ISR anidadas, EOI temprano, reloj de
servicio de dispositivos, bitácora, osciloscopio e interfaz raylib.

Cada solicitud se identifica hasta que la ISR atiende su resultado. La CPU
reanuda el mismo flujo interrumpido. Se retiraron el temporizador de
interrupciones, el quantum, los procesos y la cola de listos.

Las pruebas utilizan solicitudes reales de la API del motor y verifican
restauración de contexto, ausencia de pérdida de eventos, capacidad, prioridades
y todas las combinaciones de variantes. Los dispositivos avanzan también
durante las ISR.

El prototipo web no representa el estado actual. La captura del manual
corresponde a la interfaz de E/S en C.
