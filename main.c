#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <pthread.h>

typedef struct Message {
    int sensor_id;    // Quem detectou o fogo original
    int fire_x;       // Coordenada X do fogo
    int fire_y;       // Coordenada Y do fogo
    char time[9];     // Horário "HH:MM:SS"
} Message;

typedef struct Sensor {
    int id;
    int x; 
    int y; 
    pthread_t thread_id; // Identificador da thread do sistema
    int active; // Flag para saber se o sensor está vivo

    struct Sensor *neighbors[4];
    // Caixa de entrada (Simples: guarda 1 mensagem por vez para processar)
    Message inbox; 
    int has_message; // Flag se tem mensagem nova (0 ou 1)
    pthread_mutex_t sensor_mutex; // Protege a própria caixa de entrada
} Sensor;

#define SMALL_GRID 3
#define WHOLE_GRID 10

#define FOREST_SIZE 30 
#define SENSOR_GRID 10 
#define SENSOR_RANGE 3 



int simulation_running = 1;

pthread_mutex_t central_mutex;
pthread_cond_t  central_cond;    // Para acordar a Central
Message         central_buffer;  // Caixa de entrada da Central
int             central_has_msg = 0;

pthread_mutex_t firefighter_mutex;
pthread_cond_t  firefighter_cond; // Para acordar o Bombeiro
Message         firefighter_target; // Onde o bombeiro deve ir
int             firefighter_busy = 0; // Se o bombeiro está trabalhando

char forest[FOREST_SIZE][FOREST_SIZE]; 
pthread_mutex_t forest_mutex;

void printColoredChar(char character, char *color)
{
    if (!strcmp(color, "green"))
    {
        printf("\033[42m %c \033[m", character);
    }
    else if (!strcmp(color, "red"))
    {
        printf("\033[41m %c \033[m", character);
    }
    else
    {
        printf(" %c ", character);
    }
}



void initiateForest() {
    // 1. Limpa a floresta com '-'
    for (int i = 0; i < FOREST_SIZE; i++) {
        for (int j = 0; j < FOREST_SIZE; j++) {
            forest[i][j] = '-';
        }
    }

    // 2. Posiciona os sensores 'T'
    // A lógica é: Em uma grade 30x30 dividida em blocos de 3x3,
    // o centro de cada bloco é 1, 4, 7, 10... (fórmula: i*3 + 1)
    for (int i = 0; i < SENSOR_GRID; i++) {
        for (int j = 0; j < SENSOR_GRID; j++) {
            int centerX = (i * 3) + 1;
            int centerY = (j * 3) + 1;
            forest[centerX][centerY] = 'T'; // [cite: 45]
        }
    }
}

// Inicializa a lista de structs de sensores (serão usadas pelas threads depois)
void initiateSensorsStructs(Sensor *sensors[SENSOR_GRID][SENSOR_GRID]) {
    int id = 1;
    for (int i = 0; i < SENSOR_GRID; i++) {
        for (int j = 0; j < SENSOR_GRID; j++) {
            Sensor *s = malloc(sizeof(Sensor));
            s->id = id++;
            s->x = (i * 3) + 1;
            s->y = (j * 3) + 1;
            sensors[i][j] = s;
        }
    }
}

void *sensor_routine(void *arg) {
    Sensor *self = (Sensor *)arg;

    while (simulation_running && self->active) {
        // --- TAREFA 1: Ler Caixa de Entrada ---
        pthread_mutex_lock(&self->sensor_mutex);
        if (self->has_message) {
            // Processar mensagem (Repassar para vizinhos)
            // Lógica: Se sou borda, aviso Central. Se não, repasso.
            // (Implementaremos a lógica de borda no próximo passo para não complicar agora)
            
            // Simulação de print debug
            // printf("Sensor %d recebeu alerta de fogo em %d,%d\n", self->id, self->inbox.fire_x, self->inbox.fire_y);
            
            self->has_message = 0; // Mensagem lida
        }
        pthread_mutex_unlock(&self->sensor_mutex);


        // --- TAREFA 2: Monitorar Floresta (O código do passo anterior) ---
        pthread_mutex_lock(&forest_mutex);
        
        // Verifica se morreu
        if (forest[self->x][self->y] == '@') {
            self->active = 0;
            pthread_mutex_unlock(&forest_mutex);
            pthread_exit(NULL);
        }

        // Verifica vizinhos
        int fire_detected = 0;
        int fx = -1, fy = -1;
        
        // ... (seu loop de verificação de vizinhos aqui) ...
        // Se achou fogo:
        if (fire_detected) {
             // Avisar todos os vizinhos conectados!
             for (int n = 0; n < 4; n++) {
                 if (self->neighbors[n] != NULL) {
                     sendMessage(self, self->neighbors[n], fx, fy);
                 }
             }
        }

        if (fire_detected) {
            // Verifica se este sensor é BORDA 
            // Borda do grid de sensores (índices 0 ou 9 na matriz de sensores)
            // Precisamos saber o índice I e J do sensor na matriz de sensores, não na floresta.
            // (Dica: Podemos calcular ou salvar na struct. Vamos calcular baseados na posição)
            int grid_i = (self->x - 1) / 3;
            int grid_j = (self->y - 1) / 3;
            
            int is_border = (grid_i == 0 || grid_i == SENSOR_GRID - 1 || 
                             grid_j == 0 || grid_j == SENSOR_GRID - 1);

            if (is_border) {
                // --- COMUNICAÇÃO COM A CENTRAL ---
                pthread_mutex_lock(&central_mutex);
                central_buffer.sensor_id = self->id;
                central_buffer.fire_x = fx;
                central_buffer.fire_y = fy;
                
                // Pega hora
                time_t now = time(NULL);
                struct tm *t = localtime(&now);
                sprintf(central_buffer.time, "%02d:%02d:%02d", t->tm_hour, t->tm_min, t->tm_sec);
                
                central_has_msg = 1;
                pthread_cond_signal(&central_cond); // ACORDA A CENTRAL!
                pthread_mutex_unlock(&central_mutex);
                
            } else {
                // --- PROPAGAÇÃO PARA VIZINHOS --- 
                // Envia para todos os vizinhos conectados
                for (int n = 0; n < 4; n++) {
                    if (self->neighbors[n] != NULL) {
                        sendMessage(self, self->neighbors[n], fx, fy);
                    }
                }
            }
        }

        pthread_mutex_unlock(&forest_mutex);
        sleep(1);
    }
    return NULL;
}

// Imprime a matriz global (muito mais simples agora)
void printForest() {
    // Bloqueia o acesso para leitura (importante quando tivermos threads rodando)
    pthread_mutex_lock(&forest_mutex);

    // Cabeçalho das colunas
    printf("   ");
    for(int j=0; j < FOREST_SIZE; j++) {
        printf("%2d ", j);
    }
    printf("\n");

    for (int i = 0; i < FOREST_SIZE; i++) {
        printf("%2d ", i); // Cabeçalho das linhas
        for (int j = 0; j < FOREST_SIZE; j++) {
            char cell = forest[i][j];
            
            if (cell == 'T') {
                printColoredChar(cell, "blue"); // Sensor
            } else if (cell == '@') {
                printColoredChar(cell, "red"); // Fogo
            } else if (cell == '-') {
                printColoredChar(cell, "green"); // Floresta
            } else {
                printf(" %c ", cell);
            }
        }
        printf("\n");
    }

    // Libera o acesso
    pthread_mutex_unlock(&forest_mutex);
}


void freeSensors(Sensor *sensors[SENSOR_GRID][SENSOR_GRID]) {
    for (int i = 0; i < SENSOR_GRID; i++) {
        for (int j = 0; j < SENSOR_GRID; j++) {
            free(sensors[i][j]);
        }
    }
}


void fire() {
    // Bloqueia para escrever
    pthread_mutex_lock(&forest_mutex);

    int r = rand() % FOREST_SIZE;
    int c = rand() % FOREST_SIZE;

    forest[r][c] = '@'; 

    pthread_mutex_unlock(&forest_mutex);
}

void *fire_generator_routine(void *arg) {
    while (simulation_running) {
        // Gera fogo a cada 5 segundos [cite: 71, 80]
        sleep(5); 

        pthread_mutex_lock(&forest_mutex);
        
        int r = rand() % FOREST_SIZE;
        int c = rand() % FOREST_SIZE;
        
        // Coloca fogo na matriz global
        forest[r][c] = '@'; 
        
        pthread_mutex_unlock(&forest_mutex);
    }
    return NULL;
}

void linkSensors(Sensor *sensors[SENSOR_GRID][SENSOR_GRID]) {
    for (int i = 0; i < SENSOR_GRID; i++) {
        for (int j = 0; j < SENSOR_GRID; j++) {
            Sensor *s = sensors[i][j];
            
            // Inicializa vizinhos como NULL
            for(int n=0; n<4; n++) s->neighbors[n] = NULL;

            // Define Vizinho de CIMA (i-1)
            if (i > 0) s->neighbors[0] = sensors[i-1][j];

            // Define Vizinho da DIREITA (j+1)
            if (j < SENSOR_GRID - 1) s->neighbors[1] = sensors[i][j+1];

            // Define Vizinho de BAIXO (i+1)
            if (i < SENSOR_GRID - 1) s->neighbors[2] = sensors[i+1][j];

            // Define Vizinho da ESQUERDA (j-1)
            if (j > 0) s->neighbors[3] = sensors[i][j-1];
        }
    }
}

void sendMessage(Sensor *sender, Sensor *receiver, int fire_x, int fire_y) {
    if (receiver == NULL || !receiver->active) return;

    // Tenta pegar o lock do vizinho para deixar um recado
    pthread_mutex_lock(&receiver->sensor_mutex);
    
    // Se o vizinho já tem mensagem não lida, ignoramos (para não travar o fluxo com filas complexas)
    // Ou sobrescrevemos, dependendo da lógica. Vamos sobrescrever para garantir alerta recente.
    receiver->inbox.sensor_id = sender->id; // Quem mandou (ou Id original)
    receiver->inbox.fire_x = fire_x;
    receiver->inbox.fire_y = fire_y;
    
    // Pega hora atual
    time_t now = time(NULL);
    struct tm *t = localtime(&now);
    sprintf(receiver->inbox.time, "%02d:%02d:%02d", t->tm_hour, t->tm_min, t->tm_sec);
    
    receiver->has_message = 1; // "Você tem correio!"
    
    pthread_mutex_unlock(&receiver->sensor_mutex);
}

void *firefighter_routine(void *arg) {
    while (simulation_running) {
        pthread_mutex_lock(&firefighter_mutex);
        while (!firefighter_busy && simulation_running) {
            pthread_cond_wait(&firefighter_cond, &firefighter_mutex);
        }
        if (!simulation_running) {
            pthread_mutex_unlock(&firefighter_mutex);
            break;
        }
        int fx = firefighter_target.fire_x;
        int fy = firefighter_target.fire_y;
        pthread_mutex_unlock(&firefighter_mutex);

        // Tempo de combate
        sleep(2);

        // Apaga fogo
        pthread_mutex_lock(&forest_mutex);

        if (forest[fx][fy] == '@') {
            forest[fx][fy] = '-'; // '/' = Apagado
        }
        pthread_mutex_unlock(&forest_mutex);

        pthread_mutex_lock(&firefighter_mutex);
        firefighter_busy = 0;
        pthread_mutex_unlock(&firefighter_mutex);
    }
    return NULL;
}

void *central_routine(void *arg) {
    FILE *logfile;

    while (simulation_running) {
        pthread_mutex_lock(&central_mutex);
        
        // Espera alerta chegar da borda
        while (!central_has_msg && simulation_running) {
            pthread_cond_wait(&central_cond, &central_mutex);
        }
        
        if (!simulation_running) {
            pthread_mutex_unlock(&central_mutex);
            break;
        }

        // Processa mensagem
        Message msg = central_buffer;
        central_has_msg = 0; // Libera buffer
        pthread_mutex_unlock(&central_mutex);

        // 1. Escreve no Log 
        logfile = fopen("incendios.log", "a");
        if (logfile) {
            fprintf(logfile, "FOGO DETECTADO! Sensor ID: %d | Local: [%d, %d] | Hora: %s\n", 
                    msg.sensor_id, msg.fire_x, msg.fire_y, msg.time);
            fclose(logfile);
        }

        // 2. Acorda o Bombeiro 
        pthread_mutex_lock(&firefighter_mutex);
        if (!firefighter_busy) { // Só chama se ele não estiver ocupado
            firefighter_target = msg;
            firefighter_busy = 1;
            pthread_cond_signal(&firefighter_cond);
        }
        pthread_mutex_unlock(&firefighter_mutex);
    }
    return NULL;
}

int main() {
    srand(time(NULL));

    if (pthread_mutex_init(&forest_mutex, NULL) != 0) {
        perror("Falha no Mutex");
        return 1;
    }

    initiateForest();

    pthread_mutex_init(&central_mutex, NULL);
    pthread_cond_init(&central_cond, NULL);
    pthread_mutex_init(&firefighter_mutex, NULL);
    pthread_cond_init(&firefighter_cond, NULL);

    // 1. Criar e iniciar as threads dos sensores [cite: 49]
    Sensor *sensors[SENSOR_GRID][SENSOR_GRID];
    int id_counter = 1;

    for (int i = 0; i < SENSOR_GRID; i++) {
        for (int j = 0; j < SENSOR_GRID; j++) {
            Sensor *s = malloc(sizeof(Sensor));
            s->id = id_counter++;
            s->x = (i * 3) + 1;
            s->y = (j * 3) + 1;
            s->active = 1;
            sensors[i][j] = s;

            // Cria a thread passando o ponteiro do próprio sensor como argumento
            pthread_create(&s->thread_id, NULL, sensor_routine, (void*)s);
                
            pthread_mutex_init(&s->sensor_mutex, NULL);
        }
    }

    linkSensors(sensors);

    pthread_t central_thread;
    pthread_create(&central_thread, NULL, central_routine, NULL);

    pthread_t firefighter_thread;
    pthread_create(&firefighter_thread, NULL, firefighter_routine, NULL);
    
    // 2. Criar a thread geradora de fogo [cite: 80]
    pthread_t fire_thread;
    pthread_create(&fire_thread, NULL, fire_generator_routine, NULL);
    

    // 3. Loop principal (Visualização) 
    // Roda por 60 segundos ou infinitamente, dependendo do gosto
    for (int k = 0; k < 20; k++) { 
        printForest();
        printf("\nTempo: %d segundos. (Fogo novo a cada 5s)\n", k);
        sleep(1); // Atualiza tela a cada 1 segundo
    }

   // 1. Avisa threads para pararem
    simulation_running = 0; 

    // 2. ACORDA quem está preso em cond_wait (CRUCIAL)
    // Se não fizer isso, o bombeiro fica esperando pra sempre e o join trava
    pthread_mutex_lock(&firefighter_mutex);
    pthread_cond_broadcast(&firefighter_cond); // Acorda o bombeiro pra ele ver que acabou
    pthread_mutex_unlock(&firefighter_mutex);

    pthread_mutex_lock(&central_mutex);
    pthread_cond_broadcast(&central_cond);     // Acorda a central se ela usar cond
    pthread_mutex_unlock(&central_mutex);

    // 3. Espera as threads terminarem (JOIN) antes de limpar a memória
    printf("Aguardando encerramento das threads...\n");
    
    pthread_join(fire_thread, NULL);
    pthread_join(firefighter_thread, NULL);
    pthread_join(central_thread, NULL);

    for (int i = 0; i < SENSOR_GRID; i++) {
        for (int j = 0; j < SENSOR_GRID; j++) {
             // O sensor precisa verificar simulation_running no loop dele para isso funcionar
             pthread_join(sensors[i][j]->thread_id, NULL);
        }
    }

    // 4. Limpeza de memória (Só agora é seguro)
    printf("Limpando memória...\n");
    for (int i = 0; i < SENSOR_GRID; i++) {
        for (int j = 0; j < SENSOR_GRID; j++) {
            pthread_mutex_destroy(&sensors[i][j]->sensor_mutex); // Destruir mutex do sensor
            free(sensors[i][j]);
        }
    }

    pthread_mutex_destroy(&forest_mutex);
    pthread_mutex_destroy(&central_mutex);
    pthread_cond_destroy(&central_cond);
    pthread_mutex_destroy(&firefighter_mutex);
    pthread_cond_destroy(&firefighter_cond);

    printf("Simulação finalizada com sucesso.\n");
    return 0;
}
