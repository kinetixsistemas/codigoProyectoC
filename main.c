#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <time.h>

#define SIZE 10

// Estructura para el Dron
typedef struct {
    int x, y;         // Posición actual en la cuadrícula
    int base_x, base_y; // Posición de la base (B)
    int bateria;      // Porcentaje de batería (0 - 100)
    float carga;      // Carga en kg
    bool volando;
    bool regresando;
} Dron;

// Estructura para los Objetivos
typedef struct {
    int x, y;
    char nombre[40];
    int prioridad;    // 1 es mayor prioridad
    bool visitado;
    bool es_colapso;  // Define si hay escombros propensos a réplica
} Objetivo;

// Estructura para el Clima
typedef struct {
    int viento;       // km/h
    int lluvia;       // mm/h
    int alerta;       // 0: NORMAL, 1: PRECAUCIÓN, 2: PELIGRO
} Clima;

// Prototipos de funciones
void inicializarGrid(char grid[SIZE][SIZE], Dron d, Objetivo objs[], int num_objs);
void imprimirGrid(char grid[SIZE][SIZE], Dron d, int turno, Clima c);
int calcularDistancia(int x1, int y1, int x2, int y2);

int main() {
    // Inicializar semilla aleatoria
    srand(time(NULL));

    // Configuración inicial del Dron
    Dron dron = {2, 2, 2, 2, 100, 3.0f, true, false};

    // Configuración de Objetivos
    Objetivo objetivos[2] = {
        {7, 3, "Edificio colapsado La Guaira", 1, false, true},
        {5, 7, "Hospital Central", 2, false, false}
    };
    int num_objetivos = 2;

    // Coordenadas fijas de obstáculos [X] y zonas de riesgo [!]
    int obstaculos[5][2] = {{4, 5}, {4, 6}, {6, 4}};
    int zonas_riesgo[1][2] = {{8, 6}};

    Clima clima = {15, 5, 0}; // Viento 15 km/h, Lluvia 5 mm/h, Normal (0)

    char grid[SIZE][SIZE];
    int turnos = 0;
    int intentos_bloqueo = 0;
    bool mision_cumplida = false;
    bool mision_abortada = false;
    char causa_aborto[100] = "";

    printf("========================================================================\n");
    printf("Misión: Entregar %.1f kg de insumos médicos en zonas afectadas por sismo\n", dron.carga);
    printf("Clima inicial: Viento %d km/h | Lluvia %d mm/h | Alerta: NORMAL\n", clima.viento, clima.lluvia);
    printf("========================================================================\n\n");

    // 1. Despegue normal o bloqueado por clima inicial
    if (clima.viento > 50 || clima.lluvia > 40) {
        printf("Despegue denegado por clima adverso. Viento o lluvia excesivos en base.\n");
        return 0;
    } else {
        printf("[Turno 0] Iniciando misión... Dron despegando desde la base (2,2).\n");
    }

    // Ciclo principal de simulación (Turnos / Ticks)
    while (!mision_cumplida && !mision_abortada && turnos < 50) {
        turnos++;

        // Actualizar variaciones climáticas aleatorias
        clima.viento += (rand() % 9) - 4; // Cambios sutiles de viento
        clima.lluvia += (rand() % 5) - 2; // Cambios sutiles de lluvia
        if (clima.viento < 0) clima.viento = 0;
        if (clima.lluvia < 0) clima.lluvia = 0;

        // Evaluar nivel de alerta climática
        if (clima.viento > 60 || clima.lluvia > 50) {
            clima.alerta = 2; // PELIGRO
        } else if ((clima.viento >= 30 && clima.viento <= 60) || (clima.lluvia >= 20 && clima.lluvia <= 50)) {
            clima.alerta = 1; // PRECAUCIÓN
        } else {
            clima.alerta = 0; // NORMAL
        }

        // Caso 9: Alerta climática PELIGRO en vuelo
        if (clima.alerta == 2 && dron.volando) {
            printf("[Turno %d] ¡ALERTA CLIMA PELIGRO! Viento %d km/h, Lluvia %d mm/h.\n", turnos, clima.viento, clima.lluvia);
            printf("Regreso forzado a base.\n");
            dron.regresando = true;
        }

        // Gestión de energía y consumo por clima/carga
        int consumo = 2;
        if (clima.alerta == 1) {
            consumo = 3; // Mayor consumo en precaución
            printf("[Turno %d] Precaución climática. Avance reducido.\n", turnos);
        }
        if (dron.carga > 0) consumo += 1; // Gasto extra por carga útil
        dron.bateria -= consumo;

        // Caso 14: Batería crítica (< 10%)
        if (dron.bateria < 10 && dron.bateria > 0) {
            printf("[Turno %d] ¡BATERÍA CRÍTICA! (%d%%). Aterrizaje de emergencia inmediato.\n", turnos, dron.bateria);
            mision_abortada = true;
            sprintf(causa_aborto, "Bateria critica (%d%%)", dron.bateria);
            break;
        }

        // Determinar objetivo activo (el de mayor prioridad no visitado)
        int obj_activo_idx = -1;
        int mayor_prio = 999;
        for (int i = 0; i < num_objetivos; i++) {
            if (!objetivos[i].visitado && objetivos[i].prioridad < mayor_prio) {
                mayor_prio = objetivos[i].prioridad;
                obj_activo_idx = i;
            }
        }

        // Si todos los objetivos fueron visitados, iniciar retorno a base
        bool todos_visitados = (obj_activo_idx == -1);
        if (todos_visitados && !dron.regresando) {
            dron.regresando = true;
            printf("[Turno %d] Todos los objetivos atendidos. Iniciando regreso a base.\n", turnos);
        }

        // Movimiento del Dron
        int destino_x = dron.base_x;
        int destino_y = dron.base_y;

        if (!dron.regresando && obj_activo_idx != -1) {
            destino_x = objetivos[obj_activo_idx].x;
            destino_y = objetivos[obj_activo_idx].y;
        }

        // Calcular siguiente paso hacia el destino (Lógica básica de aproximación)
        int siguiente_x = dron.x;
        int siguiente_y = dron.y;

        if (dron.x < destino_x) siguiente_x++;
        else if (dron.x > destino_x) siguiente_x--;
        else if (dron.y < destino_y) siguiente_y++;
        else if (dron.y > destino_y) siguiente_y--;

        // Verificar si la celda siguiente es un obstáculo [X]
        bool es_obstaculo = false;
        for (int i = 0; i < 3; i++) {
            if (obstaculos[i][0] == siguiente_x && obstaculos[i][1] == siguiente_y) {
                es_obstaculo = true;
                break;
            }
        }

        if (es_obstaculo) {
            intentos_bloqueo++;
            printf("[Turno %d] Obstáculo detectado en (%d,%d). Buscando ruta alternativa...\n", turnos, siguiente_x, siguiente_y);
            // Intentar desviar una celda lateralmente
            if (siguiente_y + 1 < SIZE) siguiente_y++;
            else if (siguiente_y - 1 >= 0) siguiente_y--;

            // Caso 5: Bloqueo total tras 3 intentos
            if (intentos_bloqueo >= 3) {
                printf("[Turno %d] Dron bloqueado. Demasiados obstáculos. Regresando a base por seguridad.\n", turnos);
                dron.regresando = true;
                intentos_bloqueo = 0;
            }
        } else {
            intentos_bloqueo = 0;
            dron.x = siguiente_x;
            dron.y = siguiente_y;
            printf("[Turno %d] Dron avanza a (%d,%d) | Bat: %d%%\n", turnos, dron.x, dron.y, dron.bateria);
        }

        // Verificar si entra en zona de riesgo [!]
        if (dron.x == zonas_riesgo[0][0] && dron.y == zonas_riesgo[0][1]) {
            printf("[Turno %d] Dron en zona de riesgo. Evaluando réplica sísmica...\n", turnos);
            int replica = rand() % 10;
            if (replica > 6) { // Probabilidad de réplica
                printf("[Turno %d] ¡RÉPLICA DETECTADA! Fuera de zona de colapso. Regresando a base por seguridad.\n", turnos);
                dron.regresando = true;
            }
        }

        // Verificar llegada a un objetivo [O]
        if (!dron.regresando && obj_activo_idx != -1 && dron.x == objetivos[obj_activo_idx].x && dron.y == objetivos[obj_activo_idx].y) {
            objetivos[obj_activo_idx].visitado = true;
            dron.carga = 0.0f; // Carga entregada
            printf("[Turno %d] Objetivo alcanzado: %s. Carga entregada.\n", turnos, objetivos[obj_activo_idx].nombre);

            // Caso 7: Réplica en zona de colapso
            if (objetivos[obj_activo_idx].es_colapso) {
                int replica_colapso = rand() % 10;
                if (replica_colapso > 4) {
                    printf("[Turno %d] ¡RÉPLICA! Dron sobre zona de colapso. ATERRIZAJE DE EMERGENCIA.\n", turnos);
                    mision_abortada = true;
                    sprintf(causa_aborto, "Replica sismica en zona de colapso");
                    break;
                }
            }
        }

        // Verificar llegada exitosa a la base [B]
        if (dron.regresando && dron.x == dron.base_x && dron.y == dron.base_y) {
            printf("[Turno %d] Dron en base. Misión finalizada.\n", turnos);
            mision_cumplida = true;
            break;
        }

        // Imprimir estado del grid actual cada 3 turnos para visualización en consola
        if (turnos % 3 == 0) {
            inicializarGrid(grid, dron, objetivos, num_objetivos);
            imprimirGrid(grid, dron, turnos, clima);
        }
    }

    // ==========================================================
    // RESUMEN FINAL DE LA MISIÓN
    // ==========================================================
    printf("\n==========================================================\n");
    printf("                  RESUMEN FINAL                           \n");
    printf("==========================================================\n");
    if (mision_cumplida) {
        printf("Estado: MISIÓN CUMPLIDA\n");
        printf("Objetivos atendidos: %d/%d\n", num_objetivos, num_objetivos);
        printf("Carga entregada: SÍ (3.0 kg)\n");
    } else if (mision_abortada) {
        printf("Estado: MISIÓN ABORTADA\n");
        printf("Causa: %s\n", causa_aborto);
        printf("Carga entregada: NO\n");
    } else {
        printf("Estado: MISIÓN FINALIZADA CON ADVERTENCIAS\n");
    }
    printf("Turnos totales: %d\n", turnos);
    printf("Batería restante: %d%%\n", dron.bateria > 0 ? dron.bateria : 0);
    printf("==========================================================\n");

    return 0;
}

// Función para inicializar la cuadrícula con los elementos fijos
void inicializarGrid(char grid[SIZE][SIZE], Dron d, Objetivo objs[], int num_objs) {
    for (int i = 0; i < SIZE; i++) {
        for (int j = 0; j < SIZE; j++) {
            grid[i][j] = ' ';
        }
    }
    // Colocar base
    grid[2][2] = 'B';
    // Colocar obstáculos [X]
    grid[4][5] = 'X';
    grid[4][6] = 'X';
    grid[6][4] = 'X';
    // Colocar zona de riesgo [!]
    grid[8][6] = '!';
    // Colocar objetivos [O]
    for (int i = 0; i < num_objs; i++) {
        if (!objs[i].visitado) {
            grid[objs[i].x][objs[i].y] = 'O';
        }
    }
    // Posición actual del Dron [D]
    grid[d.x][d.y] = 'D';
}

// Función para mostrar la cuadrícula formateada en consola
void imprimirGrid(char grid[SIZE][SIZE], Dron d, int turno, Clima c) {
    printf("\n--- VISUALIZACIÓN DE CUADRICULA [Turno %d] ---\n", turno);
    printf(" Clima -> Viento: %d km/h | Lluvia: %d mm/h | Alerta: %s\n", 
           c.viento, c.lluvia, 
           c.alerta == 2 ? "PELIGRO" : (c.alerta == 1 ? "PRECAUCIÓN" : "NORMAL"));
    
    printf("   ");
    for (int j = 0; j < SIZE; j++) printf(" %d ", j);
    printf("\n");

    for (int i = 0; i < SIZE; i++) {
        printf(" %d ", i);
        for (int j = 0; j < SIZE; j++) {
            printf("[%c]", grid[i][j]);
        }
        printf("\n");
    }
    printf("---------------------------------------------\n");
}