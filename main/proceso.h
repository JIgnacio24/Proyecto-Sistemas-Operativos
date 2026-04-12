#include <stdio.h>
#include <BloqueControlProceso.h>

typedef struct {
    int num; // Contenido
    BloqueControlProceso bloqueControlProceso;
} Proceso; // Estructura para representar un proceso