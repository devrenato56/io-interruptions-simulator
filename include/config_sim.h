/* Parametros del modelo de E/S dirigida por interrupciones. */
#ifndef CONFIG_SIM_H
#define CONFIG_SIM_H

#define DEV_DISCO    0
#define DEV_TECLADO  1
#define N_DISPOS     2

/* Limite de solicitudes sin atender por dispositivo, incluida la ISR. */
#define MAX_SOLICITUDES 16

/* Menor numero = mayor prioridad. Vectores del modelo didactico. */
extern const char *const DEV_NOMBRE[N_DISPOS];
extern const int DEV_PRIO[N_DISPOS];
extern const int DEV_VEC[N_DISPOS];
extern const int DEV_SERV[N_DISPOS];

#define N_ETAPAS 7
#define SIM_LOG_MAX 60

#endif
