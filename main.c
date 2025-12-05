#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>

#define SMALL_GRID 3
#define WHOLE_GRID 10

typedef struct Message
{
    int sensor_id;
    int cordx,cordy;
    char time[9];
    int active;
} Message;

typedef struct Sensor
{
    int id;
    pthread_t threadId;
    pthread_mutex_t lock;
    char matrix[SMALL_GRID][SMALL_GRID];
    int posX, posY; // visualização na grid maior

    int cordX, cordY; // calcular vizinhos
    Message inbox;

} Sensor;

Sensor *sensors[WHOLE_GRID][WHOLE_GRID]; // declaração global da grid de sensores

typedef struct CentralControl
{
    Message inbox;
    int alert_active;
    pthread_mutex_t lock;
    pthread_cond_t cond;  // controlar funcionamento da central

} CentralControl;

CentralControl centralControl; // declaração global da central de controle

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


void getCurrentTime(char *buffer)
{
    time_t now = time(NULL);
    struct tm *t = localtime(&now);
    strftime(buffer, 9, "%H:%M:%S", t);
}

void sendMessage(Sensor *vizinho, Message msg)
{
    if(vizinho == NULL) //caso esteja na borda da grid
        return;

    // trylock utilized cause thread can lock each neighbor while trying to send messages
    if(pthread_mutex_trylock(&vizinho->lock) == 0)
    {
        if(vizinho->inbox.active == 0)
        {
            vizinho->inbox = msg;
            //printf("Sensor %d avisou Sensor %d sobre fogo!\n", msg.sensor_id, vizinho->id);
        }
        pthread_mutex_unlock(&vizinho->lock);
    }

}

Sensor *initiateSensor(int id, int positionX, int positionY, int cordX, int cordY)
{
    Sensor *sensor = malloc(sizeof(Sensor));
    if (sensor == NULL)
    {
        fprintf(stderr, "Memory allocation of sensor id: %d failed\n", id);
        return NULL;
    }
    sensor->id = id;
    sensor->posX = positionX;
    sensor->posY = positionY;

    sensor->cordX = cordX;
    sensor->cordY = cordY;
    sensor->inbox.active = 0; //começa com a caixa de mensagem vazia

    for (int i = 0; i < SMALL_GRID; i++)
    {
        for (int j = 0; j < SMALL_GRID; j++)
        {
            if (i == 1 && j == 1)
            {
                sensor->matrix[i][j] = 'T';
            }
            else
            {
                sensor->matrix[i][j] = '-';
            }
        }
    }

    pthread_mutex_init(&sensor->lock, NULL);

    return sensor;
}

void *sensorThread(void *arg)
{
    // cast arg from void to Sensor
    Sensor *sensor = (Sensor *)arg;

    while (1)
    {
        pthread_mutex_lock(&sensor->lock);
        
        for (size_t i = 0; i < SMALL_GRID; i++)
        {
            for (size_t j = 0; j < SMALL_GRID; j++)
            {
                if (sensor->matrix[i][j] == '@' && (i == 1 && j == 1))
                    pthread_cancel(sensor->threadId);
                else if (sensor->matrix[i][j] == '@')
                {
                    // function to send signal to neighbours
                    Message msg;
                    msg.sensor_id = sensor->id;
                    msg.cordx = (sensor->cordX * SMALL_GRID) + i;
                    msg.cordy = (sensor->cordY * SMALL_GRID) + j;
                    getCurrentTime(msg.time);
                    msg.active = 1;

                    int x = sensor->cordX;
                    int y = sensor->cordY;
                    
                    if(x>0) // de cima
                        sendMessage(sensors[x-1][y], msg); 

                    if(x<WHOLE_GRID-1) // de baixo
                        sendMessage(sensors[x+1][y], msg); 

                    if(y>0) // da esquerda
                        sendMessage(sensors[x][y-1], msg);

                    if(y<WHOLE_GRID-1) // da direita
                        sendMessage(sensors[x][y+1], msg);
                }

                if(sensor->inbox.active == 1) // propaga a mensagem recebida
                {
                    int x = sensor->cordX;
                    int y = sensor->cordY;

                    if(x>0) // de cima
                        sendMessage(sensors[x-1][y], sensor->inbox); 

                    if(x<WHOLE_GRID-1) // de baixo
                        sendMessage(sensors[x+1][y], sensor->inbox); 

                    if(y>0) // da esquerda
                        sendMessage(sensors[x][y-1], sensor->inbox);

                    if(y<WHOLE_GRID-1) // da direita
                        sendMessage(sensors[x][y+1], sensor->inbox);
                    
                    if (x==0 || x==WHOLE_GRID-1 || y==0 || y==WHOLE_GRID-1) // se estiver na borda, manda mensagem para central
                    {
                       pthread_mutex_lock(&centralControl.lock);
                       
                       centralControl.inbox = sensor->inbox;
                       centralControl.alert_active = 1;

                       pthread_cond_signal(&centralControl.cond);

                       pthread_mutex_unlock(&centralControl.lock);
                    }

                    sensor->inbox.active = 0; 

                }
            }
        }
        pthread_mutex_unlock(&sensor->lock);

        sleep(1);
    }

    return NULL;
}

void printSensor(Sensor *sensor)
{
    for (int i = 0; i < SMALL_GRID; i++)
    {
        for (int j = 0; j < SMALL_GRID; j++)
        {
            printColoredChar(sensor->matrix[i][j], "green");
        }
        printf("\n");
    }
}

Sensor initiateGrid(Sensor *grid[WHOLE_GRID][WHOLE_GRID])
{
    int id = 0;
    for (size_t i = 0; i < WHOLE_GRID; i++)
    {
        for (size_t j = 0; j < WHOLE_GRID; j++)
        {
            id++;

            grid[i][j] = initiateSensor(id, i, j,i,j);

            Sensor *sensorPtr = grid[i][j];
            // thread adress, NULL (lib conv), function the thread is going to execute, arg to receive cast
            int status = pthread_create(&sensorPtr->threadId, NULL, sensorThread, (void *)sensorPtr);

            if (status != 0)
            {
                printf("Error initiating thread, id: %d\n", id);
            }
        }
    }
}

void freeGrid(Sensor *grid[WHOLE_GRID][WHOLE_GRID])
{
    for (size_t i = 0; i < WHOLE_GRID; i++)
    {
        for (size_t j = 0; j < WHOLE_GRID; j++)
        {
            free(grid[i][j]);
        }
    }
}

void printSensorGrid(Sensor *grid[WHOLE_GRID][WHOLE_GRID])
{
    size_t iIdx = 0, jIdx = 0;
    // printing horizontal coordinates to better visualization
    printf("%5s", ""); // 5 was chosen because the rows are formatted as %4ld and the first is always zero
    for (jIdx; jIdx < SMALL_GRID * WHOLE_GRID; jIdx++)
        printf("%2ld ", jIdx);
    printf("\n");

    for (size_t j = 0; j < WHOLE_GRID; j++)
    {
        for (size_t jSensor = 0; jSensor < SMALL_GRID; jSensor++)
        {
            for (size_t i = 0; i < WHOLE_GRID; i++)
            {
                // lock sensor before accessing it avoiding race condition
                pthread_mutex_lock(&grid[i][j]->lock);
                
                // printing vertical coordinates to better visualization
                if (i == 0)
                    printf("%4ld ", iIdx++);
                for (size_t iSensor = 0; iSensor < SMALL_GRID; iSensor++)
                {
                    char currentCell = grid[i][j]->matrix[iSensor][jSensor];
                    if (currentCell == '@')
                        printColoredChar(grid[i][j]->matrix[iSensor][jSensor], "red");
                    else
                        printColoredChar(grid[i][j]->matrix[iSensor][jSensor], "green");
                }
                pthread_mutex_unlock(&grid[i][j]->lock);
            }
            printf("\n");
        }
    }
}

void fire(Sensor *grid[WHOLE_GRID][WHOLE_GRID])
{
    const int MAX_COORD = SMALL_GRID * WHOLE_GRID;

    int global_row = rand() % MAX_COORD;
    int global_col = rand() % MAX_COORD;

    // int division to get the grid coordinate
    int grid_row = global_row / SMALL_GRID;
    int grid_col = global_col / SMALL_GRID;

    // getting the module results gives us a number from 0 to 2
    int sensor_row = global_row % SMALL_GRID;
    int sensor_col = global_col % SMALL_GRID;

    
    pthread_mutex_lock(&grid[grid_row][grid_col]->lock);
    // put fire on cell ('@')
    grid[grid_row][grid_col]->matrix[sensor_row][sensor_col] = '@';
    
    pthread_mutex_unlock(&grid[grid_row][grid_col]->lock);
}

void *centralThread(void *arg)
{
    FILE *arquivo = fopen("incendios.log", "w");

    while (1)
    {
        pthread_mutex_lock(&centralControl.lock);

        while (centralControl.alert_active == 0)
        {
            pthread_cond_wait(&centralControl.cond, &centralControl.lock);
        }

        // process alert
        fprintf(arquivo,"Fogo detectado %d at (%d,%d) at time %s\n",
               centralControl.inbox.sensor_id,
               centralControl.inbox.cordx,
               centralControl.inbox.cordy,
               centralControl.inbox.time);

        centralControl.alert_active = 0; 

        pthread_mutex_unlock(&centralControl.lock);
    }
    fclose(arquivo);

    return NULL;
}


int main(int argc, char const *argv[])
{
    srand(time(NULL));
    
    pthread_mutex_init(&centralControl.lock, NULL);
    pthread_cond_init(&centralControl.cond, NULL);
    centralControl.alert_active = 0;

    pthread_t central;
    pthread_create(&central, NULL, centralThread, NULL);


    initiateGrid(sensors);

    int fire_timer = 0;
    while (1)
    {
        if (fire_timer % 5 == 0)
           fire(sensors);
        
        system("clear");
        printSensorGrid(sensors);
        sleep(1);
        fire_timer++;
    }
    
    freeGrid(sensors);
    return 0;
}