#include <stdio.h>
#include <time.h>

//Estructura BloqueControlProceso
typedef struct {
    // Variables del Documento
    int horaLlegada;
    int cantInteraciones;
    int avancePilaCodigo;
    int tiempoEsperaES;
    int promedioTiempoES;
    int promedioEsperaListo;
    
   
    // Variables del Proceso
    int idProceso; // Identificador del proceso
    int estado; // Estado de la colmena (LISTO, EJECUTANDO, BLOQUEADO, FINALIZADO)
    int tiempoEjecucion; // Tiempo total en ejecución
    int cantAbejasActivas; // Cantidad de abejas activas
    int cantHuevosEnEclosion; // Huevos en proceso de eclosión
    int cantMielProducida; // Total de miel producida
    int cantPolenRecolectado; // Total de polen recolectado
    int tiempoTotalEjecucion;
    int tiempoRestante;
    int quantumAsignado;

    time_t ultActualizacion; // Última actualización del estado

    int prioridad; // Prioridad en la planificación
}BloqueControlProceso;