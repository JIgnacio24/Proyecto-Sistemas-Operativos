#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>//libreria que maneja hilos
#include <unistd.h>
#include <time.h> //libreria para generar numeros en random  :)
#include <stdbool.h>//libreria para el uso de booleanos
#include <signal.h>
#include <sys/types.h>//manejo de memoria compartida y archivos
#include <sys/mman.h>//manejo de memoria compartida y archivos
#include <sys/stat.h>//manejo de memoria compartida y archivos
#include <fcntl.h>//manejo de memoria compartida y archivos
#include <string.h>
#include <stdarg.h>
// #include "GestionMiel.h"
// Definiciones generales (en el pre procesador, no tienen tipo)
#define MAX_COLMENAS 40 //define el maximo de colmenas como constante 40
#define FILAS_COLMENA 10 //para definir el tamaño de las colmenas
#define COLUMNAS_COLMENA 10 //para definir el tamaño de las colmenas
#define MAX_ABEJAS 40 //define el maximo de abejas
#define MIN_ABEJAS 20 //define el minimo de abejas
#define MAX_HUEVOS 40 //cantidad maxima de huevos por colmena
#define MIN_HUEVOS 20 //cantidad minima de huevos por colmena
#define MAX_VIDA_ABEJA 5 //vida maxima de la abeja
#define MIN_VIDA_ABEJA 1 //minimo de vida de la abeja
#define UMBRAL_PROMEDIO 10 //cuando el promedio de abejas y miel llega a este valor cambia de politica


// Variables globales
int cantidad_colmenas = 0; //variable para llevar la cantidad de colmenas que hay en el momento
int colmena_actual = 0; //indice de la colmena que se esta procesando en RR.
float promedio_polen_abejas = 0.0;
int politica = 0; // 0 = Round Robin, 1 = Shortest Job First (SJF)
pthread_mutex_t mutex;
FILE *archvioProyecto;

void inicializarArchivo() {
    archvioProyecto = fopen("proyectoSO_IF4001.txt", "w");
    if (archvioProyecto == NULL) {
        perror("Error al abrir el archivo");
    }
}

void escribirEnArchivo(const char *nombreArchivo, const char *formato, ...) {
    // Abrir el archivo en modo "append"
    FILE *file = fopen(nombreArchivo, "a");

    // Verificar si el archivo se abrió correctamente
    if (file == NULL) {
        perror("Error al abrir el archivo");
        return;
    }

    // Procesar los argumentos variables
    va_list args;
    va_start(args, formato);
    vfprintf(file, formato, args); // Escribir la cadena formateada en el archivo
    va_end(args);

    // Cerrar el archivo
    fclose(file);
}

// Estructura para manejar el control de cada abeja
typedef struct {
    int id;               // ID de la abeja
    int vida;             // Vida restante
    char tipo[10];         //Tipo de abjea(Reina/obrera)
    int iteraciones_restantes; //antes de morir
    int polen_total_recolectado; //cantidad de polen recolectado
    int polen_transportando; //cantiadad de polen que tiene en el momento
    int polen_maximo; //limite de polen que puede transportar 
} ControlAbeja; // ControlAbeja

// Estructura de colmena
typedef struct {
    int identificador; //identificador unico de la
    int celdas[FILAS_COLMENA][COLUMNAS_COLMENA]; //tamaño de la colmena
    int cant_inicial_abejas; //cantidad de abejas con la que se crea la colmena
    int abejas_vivas; //cantidad de abejas vivas 
    int abejas_obreras; //cantidad de obejas de tipo obreras
    int abejas_exploradoras; //cantidad de obejas  de tipo obrer
    int huevos_actuales;
    int miel_total;
    int polen_total;
    bool reina_presente;
    pthread_t hilo_colmena;
    pid_t proceso_colmena;
    ControlAbeja abejas[MAX_ABEJAS];
} Colmena; // Colmena

// Tabla de procesos para estadísticas
typedef struct {
    int quantum;
    double promedio_abejas;
    double promedio_miel;
    double promedio_huevos;
} Estadisticas; // Estadisticas


// Variables globales
Colmena *colmenas;
Estadisticas *tabla_estadisticas;

// Declaración de funciones para que no haya problema en 
void inicializar_memoria_colmenas();
void iniciar_colmena(int id_colmena);
void crear_colmena();
void recolectar_polen(ControlAbeja *abeja, Colmena *colmena);
void producir_miel(Colmena *colmena);
void manejar_huevos(Colmena *colmena);
void planificador_politicas();
void planificacion_round_robin();
void planificacion_FSJ();
void mostrar_colmena(Colmena *colmena);
void imprimir_estado_colmena(Colmena *colmena);
void *planificador();
void calcular_promedio(void);

// void inicializarMatriz(int matriz[TAMCELDAS][TAMCELDAS]);
// void almacenarMiel(int matriz[TAMCELDAS][TAMCELDAS], int *mielProducida);
// void imprimirMatriz(int matriz[TAMCELDAS][TAMCELDAS]);
void ejecutarProduccionMiel();
// void actividadAbeja(ControlAbeja *abeja, int *polenTotal, int *mielProducida, int matriz[TAMCELDAS][TAMCELDAS]);


int main() {
    inicializarArchivo();
    srand(time(NULL));//semilla para los números random
    
    // Inicializar memoria compartida para las colmenas
    inicializar_memoria_colmenas();

    // Inicializar tabla de estadísticas
    tabla_estadisticas = (Estadisticas *)malloc(sizeof(Estadisticas));
    tabla_estadisticas->quantum = 2;

    // Crear colmenas iniciales
    for (int i = 0; i < 5; i++) {
        crear_colmena();
    }

    // Ejecutar el planificador
    planificador();

    //Miel abejas
    //ejecutarProduccionMiel();

    // Liberar memoria al final
    free(tabla_estadisticas);
    munmap(colmenas, MAX_COLMENAS * sizeof(Colmena));

    return 0;
} // main


void inicializar_memoria_colmenas() {
    
    size_t tamano_memoria = MAX_COLMENAS * sizeof(Colmena);//calcula el tamaño total de memoria necesaria para almacenar las colmenas.

    // Abre un archivo temporal para asociarlo con la memoria compartida.
    // O_CREAT: crea el archivo si no existe.
    // O_RDWR: permite lectura y escritura.
    // 0666: permisos para lectura y escritura para todos los usuarios.
    int archivo_memoria = open("/tmp/colmenas_memoria", O_CREAT | O_RDWR, 0666);

    // Ajusta el tamaño del archivo al tamaño calculado.
    ftruncate(archivo_memoria, tamano_memoria);

    // Mapea el archivo en memoria para permitir su uso como memoria compartida.
    // NULL: deja que el sistema elija la dirección de mapeo.
    // tamano_memoria: define el tamaño del segmento de memoria mapeado.
    // PROT_READ | PROT_WRITE: permisos para leer y escribir en la memoria.
    // MAP_SHARED: los cambios en la memoria son visibles para otros procesos que usen este archivo.
    // archivo_memoria: descriptor de archivo del archivo a mapear.
    // 0: offset inicial del archivo para el mapeo.
    colmenas = mmap(NULL, tamano_memoria, PROT_READ | PROT_WRITE, MAP_SHARED, archivo_memoria, 0);

    // Cierra el descriptor de archivo, ya que no se necesita más después del mapeo.
    close(archivo_memoria);
} // inicializar_memoria_colmenas


void iniciar_colmena(int id_colmena) {
    
    Colmena *colmena = &colmenas[id_colmena];

    colmena->cant_inicial_abejas = ((rand() % (MAX_ABEJAS - MIN_ABEJAS + 1)) + MIN_ABEJAS)-1;//establece la cantidad de abejas en la colmena
    colmena->abejas_obreras = (rand() % (colmena->cant_inicial_abejas - 0 + 1)) + 0;//establace un número random de la cantidad establecida para las obreras
    colmena->abejas_exploradoras = colmena->cant_inicial_abejas - colmena->abejas_obreras;//de la cantidad total se restan las obreras y esas son las exploradoras

    colmena->identificador = id_colmena;
    colmena->abejas_vivas = colmena->abejas_obreras + colmena->abejas_exploradoras;//las abejas vivas serán el total de las obreras y exploradoras, junto con la reina
    colmena->huevos_actuales = (rand() % (MAX_HUEVOS - MIN_HUEVOS + 1)) + MIN_HUEVOS; //un número aleatorio para los huevos
    colmena->miel_total = 0;//se inicializa la miel en cero
    colmena->polen_total = 0;//se inicializa el polen en cero
    colmena->reina_presente = false;//se establece que la reina aún no nace
    memset(colmena->celdas, 0, sizeof(colmena->celdas));

    for (int i = 0; i < colmena->abejas_vivas; i++) {//para cada abeja viva se inicializan los atributos
            colmena->abejas[i].iteraciones_restantes = (rand() % (MAX_VIDA_ABEJA - MIN_VIDA_ABEJA + 1)) + MIN_VIDA_ABEJA;//aún no busca polen
            colmena->abejas[i].polen_maximo = 0;//aún no tiene polen
            colmena->abejas[i].polen_total_recolectado = 0;//aún no recolecta polen
            colmena->abejas[i].polen_transportando = 0;//aún no lleva polen
            strcpy(colmena->abejas[i].tipo, "obrera");

    } // for
    
    // TEFI HAZ LO TUYO :)
    // :D
    //Se muestra en pantalla la colmena creada
    printf("Colmena creada [ID: %d] con %d abejas vivas y %d huevos.\n", colmena->identificador, colmena->abejas_vivas, colmena->huevos_actuales);
    escribirEnArchivo(
    "proyectoSO_IF4001.txt", // Nombre del archivo
    "Colmena creada [ID: %d] con %d abejas vivas y %d huevos.\n", 
    colmena->identificador, 
    colmena->abejas_vivas, 
    colmena->huevos_actuales
    );
} // iniciar_colmena

void crear_colmena() {//se crea la colmena si la cantidad actual es menor a la cantidad máxima
    if (cantidad_colmenas >= MAX_COLMENAS) {
        printf("No se pueden crear más colmenas.\n");
    escribirEnArchivo(
        "proyectoSO_IF4001.txt", // Nombre del archivo
        "No se pueden crear más colmenas.\n"
    );
        return;
    } // if
    //si aún no llega al límite se crea la colmena
    pid_t proceso = fork();
    if (proceso == 0) {
        iniciar_colmena(cantidad_colmenas);
        exit(0);
    } else if (proceso > 0) {
        cantidad_colmenas++;
    } // if-else
} // crear_colmena

/*
// Función para inicializar la matriz de colmenas con ceros
void inicializarMatriz(int matriz[TAMCELDAS][TAMCELDAS]) {
    for (int i = 0; i < TAMCELDAS; i++) {
        for (int j = 0; j < TAMCELDAS; j++) {
            matriz[i][j] = 0;
        }
    }
}
*/
/*
// Función para imprimir la matriz de la cámara
void imprimirMatriz(int matriz[TAMCELDAS][TAMCELDAS]) {
    for (int i = 0; i < TAMCELDAS; i++) {
        for (int j = 0; j < TAMCELDAS; j++) {
            printf("%2d ", matriz[i][j]);
        }
        printf("\n");
    }
}
*/
/*
// Función para agregar miel a la matriz respetando las restricciones
void almacenarMiel(int matriz[TAMCELDAS][TAMCELDAS], int *mielProducida) {
    //recorrer matriz
    for (int i = 0; i < TAMCELDAS; i++) {
        for (int j = 0; j < TAMCELDAS; j++) {
            // Validar si la celda está en la primera/última fila o columna
            if (i == 0 || i == TAMCELDAS - 1 || j == 0 || j == TAMCELDAS - 1) {
                // Verificar espacio disponible en la celda, menor
                int espacioDisponible = UNIDADES_MIEL_CELDA - matriz[i][j];//UNIDADES_CELDA es 10, menos el contenido de la matriz, se sabe si puede o no agregar la miel
                if (*mielProducida > 0 && espacioDisponible > 0) {
                    //Si hay más miel pendiente que sería *mielProducida, que espacio disponible, solo se coloca lo que cabe en la celda.
                    int mielAColocar = (*mielProducida > espacioDisponible) ? espacioDisponible : *mielProducida;
                    matriz[i][j] += mielAColocar;
                    *mielProducida -= mielAColocar;//Resta la cantidad de miel colocada de la miel pendiente por almacenar.
                    printf("Miel agregada: %d unidades en celda [%d][%d]\n", mielAColocar, i, j);//indica al usuario
                }
            }
        }
    }
}
*/
/*
// Función para simular la actividad de una abeja
void actividadAbeja(ControlAbeja *abeja, int *polenTotal, int *mielProducida, int matriz[TAMCELDAS][TAMCELDAS]) {
    if (strcmp(abeja->tipo, "reina") == 0) {//identificar si es reina
        printf("Abeja %d (reina) no recolecta polen.\n", abeja->id);//indica que no recolecta polen
        escribirEnArchivo(
            "archivoProyecto.txt", // Nombre del archivo
            "Abeja %d (reina) no recolecta polen.\n",
            abeja->id
        );

        return;
    }
*/
/*
    // Recolectar polen mientras la abeja tenga vida
    if (abeja->vida > 0) {
        int polenRecolectado = (rand() % 5) + 1;//asignación aleatoria del polen
        *polenTotal += polenRecolectado;//se le suma al polen total
        abeja->vida -= polenRecolectado;//se le resta la vida a las abejas
        printf("Abeja %d (obrera) recolectó %d gramos de polen. Vida restante: %d\n",
               abeja->id, polenRecolectado, abeja->vida);//se le indica al usuario
        escribirEnArchivo(
            "archivoProyecto.txt", // Nombre del archivo
            "Abeja %d (obrera) recolectó %d gramos de polen. Vida restante: %d\n",
            abeja->id,
            polenRecolectado,
            abeja->vida
        );
        // Convertir polen en miel si es suficiente
        if (*polenTotal >= 10) {//si hay suficiente polen para hacer miel
            int mielGenerada = *polenTotal / 10;//se realiza
            *mielProducida += mielGenerada;//se agrega al total
            *polenTotal %= 10;//se guarda el sobrante
            printf("Se generaron %d unidades de miel. Miel total pendiente por almacenar: %d unidades\n",
                   mielGenerada, *mielProducida);//se indica al usuario
            escribirEnArchivo(
                "archivoProyecto.txt", // Nombre del archivo
                "Se generaron %d unidades de miel. Miel total pendiente por almacenar: %d unidades\n",
                mielGenerada,
                *mielProducida
            );
        }

        // Almacenar la miel en la matriz
        //almacenarMiel(matriz, mielProducida);
    } else {
        printf("Abeja %d (obrera) ha alcanzado su límite de vida y ya no puede recolectar.\n", abeja->id);//si una abeja ya recolectó las 150 unidades, muere
        escribirEnArchivo(
            "archivoProyecto.txt", // Nombre del archivo
            "Abeja %d (obrera) ha alcanzado su límite de vida y ya no puede recolectar.\n",
            abeja->id
            );
    }
}
*/
void recolectar_polen(ControlAbeja *abeja, Colmena *colmena) {
    while (abeja->iteraciones_restantes > 0) {//para recolectar el polen, se revisa que l
    // abeja que pasa aún no llega al límite de iteraciones        
 int polen_recolectado = rand() % abeja->polen_maximo;
        abeja->polen_transportando = polen_recolectado;
//se estan       abeja->polen_total_recolectado += polen_recolectado;

        printf("Abeja recolectando %d de polen en colmena %d.\n", polen_recolectado, colmena->identificador);

        for (int i = 0; i < FILAS_COLMENA; i++) {
            for (int j = 0; j < COLUMNAS_COLMENA; j++) {
                if (colmena->celdas[i][j] == 0) {
                    colmena->celdas[i][j] = 1;
                    colmena->polen_total += polen_recolectado;
                    printf("Polen depositado en celda [%d][%d] de colmena %d.\n", i, j, colmena->identificador);
                    break;
                } // if
            } // for
        } // for
        abeja->iteraciones_restantes--;
    } // while
} // recolectar_polen

void producir_miel(Colmena *colmena) {
    if (colmena->polen_total >= 10) {
        colmena->polen_total -= 10;
        colmena->miel_total++;
        printf("Colmena %d convirtió polen en miel. Total de miel: %d.\n", colmena->identificador, colmena->miel_total);
        escribirEnArchivo("proyectoSO_IF4001.txt", 
            "Colmena %d convirtió polen en miel. Total de miel: %d.\n", 
            colmena->identificador, colmena->miel_total
            );

    } else {
        printf("Colmena %d no tiene suficiente polen para producir miel.\n", colmena->identificador);
        escribirEnArchivo("proyectoSO_IF4001.txt", 
            "Colmena %d no tiene suficiente polen para producir miel.\n", 
             colmena->identificador
             );

    } // if-else
} // producir_miel

void manejar_huevos(Colmena *colmena) {
    if (colmena->huevos_actuales > 0) {
        colmena->huevos_actuales--;
        colmena->abejas_vivas++;
        printf("Un huevo eclosionó en colmena %d. Nuevas abejas vivas: %d.\n", colmena->identificador, colmena->abejas_vivas);
        // Supongamos que tienes un puntero a colmena llamado 'colmena' con los campos 'identificador' y 'abejas_vivas'
        escribirEnArchivo("proyectoSO_IF4001.txt", 
            "Un huevo eclosionó en colmena %d. Nuevas abejas vivas: %d.\n", 
            colmena->identificador, colmena->abejas_vivas
        );

    } // if

    if (!colmena->reina_presente && rand() % 2 == 0) {
        colmena->reina_presente = true;
        printf("¡Abeja reina nacida en colmena %d! Se creará una nueva colmena.\n", colmena->identificador);
        escribirEnArchivo("proyectoSO_IF4001.txt", 
            "¡Abeja reina nacida en colmena %d! Se creará una nueva colmena.\n", 
            colmena->identificador
        );

        crear_colmena();
    } // if
} // manejar_huevos*/

// Función para calcular el promedio de polen y abejas
void calcular_promedio() {
    int total_abejas = 0;
    int total_miel = 0;

    pthread_mutex_lock(&mutex);
    for (int i = 0; i < MAX_COLMENAS; i++) {
        total_abejas += colmenas[i].abejas_vivas;
        total_miel += colmenas[i].miel_total;
    } // for
    
    promedio_polen_abejas = (total_abejas + total_miel) / (float)(2 * MAX_COLMENAS);
    pthread_mutex_unlock(&mutex);

    printf("Promedio de polen y abejas: %.2f\n", promedio_polen_abejas);
    escribirEnArchivo(
        "proyectoSO_IF4001.txt", // Nombre del archivo
        "Promedio de polen y abejas: %.2f\n",
        promedio_polen_abejas
    );

} // calcular_promedio

// Hilo del planificador
void *planificador(void *arg) {//si hay que ser minero, romper el pico en el hierro, no importa creeper que venga pa que sepas que te quiero como un buen, mineroooooooooo, me juego la vida por tiiiiiiiiiii, si hay que ser minero, romper el pico en el hierro, no importa
    while (1) {
        // Calcular promedio
        calcular_promedio();

        // Cambiar política si el promedio supera el umbral
        pthread_mutex_lock(&mutex);
        if (promedio_polen_abejas > UMBRAL_PROMEDIO) {
            politica = 1 - politica; // Cambiar política
            printf("Cambio de política: ahora es %s\n", politica == 0 ? "Round Robin" : "SJF");
            escribirEnArchivo(
                "proyectoSO_IF4001.txt", // Nombre del archivo
                "Cambio de política: ahora es %s\n",
                politica == 0 ? "Round Robin" : "SJF"
            );
        } // if
        pthread_mutex_unlock(&mutex);

        // Ejecutar la política actual
        pthread_mutex_lock(&mutex);
        if (politica == 0) {
            printf("Ejecutando Round Robin\n");
            escribirEnArchivo(
                "proyectoSO_IF4001.txt", // Nombre del archivo
                "Ejecutando Round Robin\n"
            );
            planificacion_round_robin();
        } else {
            printf("Ejecutando Shortest Job First (SJF)\n");
            escribirEnArchivo(
                "proyectoSO_IF4001.txt", // Nombre del archivo
                "Ejecutando Shortest Job First (SJF)\n"
            );
            planificacion_FSJ();
        } // if-else
        pthread_mutex_unlock(&mutex);

        sleep(5); // Esperar antes de reevaluar
    } // while
    return NULL;
} // planificador

void ajustar_quantum() { //esto me lo dio chat, hay que revisarlo a ver si funciona, junto con las 2 funciones de abajo
    const int quantum_min = 2;
    const int quantum_max = 10;

    if (tabla_estadisticas->promedio_huevos < 5 || tabla_estadisticas->promedio_miel < 5) {
        // Si la producción promedio es baja, aumentar quantum para dar más tiempo.
        tabla_estadisticas->quantum += 2;
    } else if (tabla_estadisticas->promedio_abejas > 10) {
        // Si hay muchas abejas, reducir quantum para alternar más rápido.
        tabla_estadisticas->quantum -= 2;
    } // if-else

    // Asegurar límites.
    if (tabla_estadisticas->quantum < quantum_min) {
        tabla_estadisticas->quantum = quantum_min;
    } else if (tabla_estadisticas->quantum > quantum_max) {
        tabla_estadisticas->quantum = quantum_max;
    } // if-else
} // ajustar_quantum

void evaluar_cambio_politica(int iteraciones) { //esto me lo dio chat con lo de arriba y abajo, hay que revisar
    if (iteraciones % 20 == 0) {
        // Cada 20 iteraciones, cambiar de política basado en métricas.
        if (tabla_estadisticas->promedio_miel < 3) {
            printf("Cambiando a política de FSJ.\n");
            escribirEnArchivo(
                "proyectoSO_IF4001.txt", // Nombre del archivo
                "Cambiando a política de FSJ.\n"
            );
            tabla_estadisticas->quantum = 4; // Ajuste del quantum para esta política.
        } else {
            printf("Manteniendo Round Robin.\n");
            escribirEnArchivo(
                "proyectoSO_IF4001.txt", // Nombre del archivo
                "Manteniendo Round Robin.\n"
            );
        } // if-else
    } // if
} // evaluar_cambio_politica

void planificador_politicas() { //esto es el planificador nuevo de chat, junto con las 2 funciones de arriba, revisar
    int iteraciones = 0; 

    // Inicializar el quantum aleatorio al principio.
    tabla_estadisticas->quantum = (rand() % (10 - 2 + 1)) + 2;

    while (1) {
        printf("\n=== Iteración del Planificador: %d ===\n", iteraciones);
        escribirEnArchivo(
            "proyectoSO_IF4001.txt", // Nombre del archivo
            "\n=== Iteración del Planificador: %d ===\n",
            iteraciones
        );
        // Cambiar de política cada 20 iteraciones.
        evaluar_cambio_politica(iteraciones);

        // Alternar entre políticas activas.
        if (iteraciones % 2 == 0) {
            printf("Ejecutando política Round Robin.\n");
            escribirEnArchivo(
                "proyectoSO_IF4001.txt", // Nombre del archivo
                "Ejecutando política Round Robin.\n"
            );
            planificacion_round_robin();
        } else {
            printf("Ejecutando política FSJ.\n");
            escribirEnArchivo(
                "proyectoSO_IF4001.txt", // Nombre del archivo
                "Ejecutando política FSJ.\n"
            );
            planificacion_FSJ();
        } // if-else

        // Ajustar el quantum cada 10 iteraciones.
        if (iteraciones % 10 == 0) {
            ajustar_quantum();
            printf("Quantum ajustado a: %d\n", tabla_estadisticas->quantum);
            escribirEnArchivo(
                "proyectoSO_IF4001.txt", // Nombre del archivo
                "Quantum ajustado a: %d\n",
                tabla_estadisticas->quantum
                );
        } // if

        sleep(tabla_estadisticas->quantum);
        iteraciones++;
    } // while
} // planificador_politicas

void planificacion_round_robin() {
    Colmena *colmena = &colmenas[colmena_actual];
    printf("Planificación Round Robin: Procesando colmena %d.\n", colmena->identificador);
    escribirEnArchivo(
        "proyectoSO_IF4001.txt", // Nombre del archivo
        "Planificación Round Robin: Procesando colmena %d.\n",
        colmena->identificador
    );
    producir_miel(colmena);
    manejar_huevos(colmena);
    colmena_actual = (colmena_actual + 1) % cantidad_colmenas;
} // planificacion_round_robin

void planificacion_FSJ() {
    int indice_menor = 0;
    for (int i = 1; i < cantidad_colmenas; i++) {
        if (colmenas[i].miel_total < colmenas[indice_menor].miel_total) {
            indice_menor = i;
        } // if
    } // for
    Colmena *colmena = &colmenas[indice_menor];
    printf("Politica FSJ: Procesando colmena %d.\n", colmena->identificador);
    escribirEnArchivo(
        "proyectoSO_IF4001.txt", // Nombre del archivo
        "Politica FSJ: Procesando colmena %d.\n",
        colmena->identificador
    );
    producir_miel(colmena);
    manejar_huevos(colmena);
} // planificacion_FSJ
