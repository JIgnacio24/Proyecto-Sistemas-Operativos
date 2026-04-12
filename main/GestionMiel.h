#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>

#define TAMCELDAS 10 //tamaño matriz
#define UNIDADES_MIEL_CELDA 10 //espacio de unidades de miel en cada celda de la matriz
#define VIDA_MAXIMA 150//vida máxima de las abejas

typedef struct {
    int id;               // ID de la abeja
    int vida;             // Vida restante
    char tipo[10];        // Tipo de abeja: "reina" u "obrera"
} Abeja;

// Función para inicializar la matriz con ceros
void inicializarMatriz(int matriz[TAMCELDAS][TAMCELDAS]) {
    for (int i = 0; i < TAMCELDAS; i++) {
        for (int j = 0; j < TAMCELDAS; j++) {
            matriz[i][j] = 0;
        }
    }
}

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

// Función para imprimir la matriz de la cámara
void imprimirMatriz(int matriz[TAMCELDAS][TAMCELDAS]) {
    for (int i = 0; i < TAMCELDAS; i++) {
        for (int j = 0; j < TAMCELDAS; j++) {
            printf("%2d ", matriz[i][j]);
        }
        printf("\n");
    }
}

// Función para simular la actividad de una abeja
void actividadAbeja(Abeja *abeja, int *polenTotal, int *mielProducida, int matriz[TAMCELDAS][TAMCELDAS]) {
    if (strcmp(abeja->tipo, "reina") == 0) {//identificar si es reina
        printf("Abeja %d (reina) no recolecta polen.\n", abeja->id);//indica que no recolecta polen
        return;
    }

    // Recolectar polen mientras la abeja tenga vida
    if (abeja->vida > 0) {
        int polenRecolectado = (rand() % 5) + 1;//asignación aleatoria del polen
        *polenTotal += polenRecolectado;//se le suma al polen total
        abeja->vida -= polenRecolectado;//se le resta la vida a las abejas
        printf("Abeja %d (obrera) recolectó %d gramos de polen. Vida restante: %d\n",
               abeja->id, polenRecolectado, abeja->vida);//se le indica al usuario

        // Convertir polen en miel si es suficiente
        if (*polenTotal >= 10) {//si hay suficiente polen para hacer miel
            int mielGenerada = *polenTotal / 10;//se realiza
            *mielProducida += mielGenerada;//se agrega al total
            *polenTotal %= 10;//se guarda el sobrante
            printf("Se generaron %d unidades de miel. Miel total pendiente por almacenar: %d unidades\n",
                   mielGenerada, *mielProducida);//se indica al usuario
        }

        // Almacenar la miel en la matriz
        almacenarMiel(matriz, mielProducida);
    } else {
        printf("Abeja %d (obrera) ha alcanzado su límite de vida y ya no puede recolectar.\n", abeja->id);//si una abeja ya recolectó las 150 unidades, muere
    }
}

int main() {
    int matriz[TAMCELDAS][TAMCELDAS];//cámaras de la colmena
    int mielProducida = 0, polenTotal = 0;
    
    inicializarMatriz(matriz);// Inicializar la matriz de la cámara
    
    srand(time(NULL));// Semilla para generar números aleatorios

    // Crear las abejas (3 obreras y 1 reina)
    Abeja abejas[4] = {
        {1, VIDA_MAXIMA, "obrera"},
        {2, VIDA_MAXIMA, "obrera"},
        {3, VIDA_MAXIMA, "obrera"},
        {4, VIDA_MAXIMA, "reina"}
    };

    // Simular actividad
    printf("Simulación de recolección y almacenamiento de miel:\n");
    for (int iteracion = 1; iteracion <= 20; iteracion++) { // 20 iteraciones
        printf("\nIteración %d:\n", iteracion);
        for (int i = 0; i < 4; i++) {
            actividadAbeja(&abejas[i], &polenTotal, &mielProducida, matriz);//indica la actividad de cada abeja, polen recolectad, miel creada y almacenada
        }
        //estado visual de la matriz
        printf("\nEstado actual de la matriz de almacenamiento:\n");
        imprimirMatriz(matriz);
        printf("Miel pendiente por almacenar: %d unidades\n", mielProducida);
        printf("--------------------------------------------------\n");
    }

    return 0;
}
