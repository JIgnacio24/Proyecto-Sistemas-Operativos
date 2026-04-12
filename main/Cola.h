#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <proceso.h>
/*
    Porción de código para la declaración de la Cola
*/
typedef struct Nodo {
    Proceso proceso;         // Proceso que contiene el nodo
    struct Nodo* siguiente;   // Puntero al siguiente nodo
} Nodo; // Nodo de Cola


typedef struct {
    Nodo* frente; // Puntero al primer nodo
    Nodo* atras;  // Puntero al último nodo
} Cola; // Cola de procesos

// Funciones para manejar la cola
void inicializarCola(Cola* cola) {
    cola->frente = NULL;
    cola->atras = NULL;
} // Inicializa la cola vacía

int esColaVacia(Cola* cola) {
    return cola->frente == NULL;
} // Verifica si la cola está vacía

void encolar(Cola* cola, Proceso proceso) {
    Nodo* nuevoNodo = (Nodo*)malloc(sizeof(Nodo));
    nuevoNodo->proceso = proceso;
    nuevoNodo->siguiente = NULL;

    if (esColaVacia(cola)) {
        cola->frente = nuevoNodo;
        cola->atras = nuevoNodo;
    } else {
        cola->atras->siguiente = nuevoNodo;
        cola->atras = nuevoNodo;
    } // if
} // Agrega un proceso a la cola

Proceso desencolar(Cola* cola) {
    if (esColaVacia(cola)) {
        fprintf(stderr, "Error: Cola vacía. No se puede desencolar.\n");
        exit(EXIT_FAILURE); // Manejo de error
    } // if
    
    Nodo* nodoAEliminar = cola->frente;
    Proceso proceso = nodoAEliminar->proceso;
    cola->frente = cola->frente->siguiente;

    if (cola->frente == NULL) {
        cola->atras = NULL; // Si la cola se vacía, también actualiza el puntero de atrás
    } // if

    free(nodoAEliminar); // Liberar la memoria del nodo eliminado
    return proceso;
} // Fin de la función desencolar
