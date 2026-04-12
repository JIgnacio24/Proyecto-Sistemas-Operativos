#include <stdio.h>
#include <time.h>
#include <proceso.h>

//Estructura TablaProceso
typedef struct {
    // Variables del Documento
    int prmedioHoraLlegada;
    int prmedioCantInteraciones;
    int prmedioAvancePilaCodigo;
    int prmedioTiempoEsperaES;
    int promedioTiempoES; // De todos las colmenas (procesos)
    int promedioEsperaListo; // De todos las colmenas (procesos)

    // Variables del Proceso
    int tiempoRestante; // Tiempo restante para completar el proceso
    int tiempoTotalEjecucion; // Tiempo total en ejecución
    int cantAbejasActivas; // Número de abejas activas en la colmena
    int cantHuevosEnEclosion; // Número de huevos en proceso de eclosión
    int cantMielProducida; // Cantidad de miel producida hasta el momento

    time_t ultActualizacion; // Última actualización del estado
}TablaProceso; // TablaProceso
