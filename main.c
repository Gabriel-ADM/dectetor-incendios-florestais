#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>

#define SMALL_GRID 3
#define WHOLE_GRID 10

// global var to control mutual exclusion
pthread_mutex_t grid_mutex = PTHREAD_MUTEX_INITIALIZER;

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

typedef struct Sensor
{
    int id;
    pthread_t threadId;
    char matrix[SMALL_GRID][SMALL_GRID];
    int posX, posY;
} Sensor;

Sensor *initiateSensor(int id, int positionX, int positionY)
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
    return sensor;
}

void *sensorThread(void *arg)
{
    // cast arg from void to Sensor
    Sensor *sensor = (Sensor *)arg;

    while (1)
    {
        sleep(1);
        // locks matrix from another thread before reading it
        pthread_mutex_lock(&grid_mutex);

        for (size_t i = 0; i < SMALL_GRID; i++)
        {
            for (size_t j = 0; j < SMALL_GRID; j++)
            {
                if (sensor->matrix[i][j] == '@' && (i == 1 && j == 1))
                {
                    // prevent deadlock
                    pthread_mutex_unlock(&grid_mutex);
                    pthread_cancel(sensor->threadId);
                    return NULL;
                }
                else if (sensor->matrix[i][j] == '@')
                {
                    // function to send signal to neighbours
                }
            }
        }
        // free thread after checking the matrix content
        pthread_mutex_unlock(&grid_mutex);
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

            grid[i][j] = initiateSensor(id, i, j);

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
            }
            printf("\n");
        }
    }
}

void fire(Sensor *grid[WHOLE_GRID][WHOLE_GRID])
{
    // prevent deadlock from main thread with sensor threads
    pthread_mutex_lock(&grid_mutex);
    const int MAX_COORD = SMALL_GRID * WHOLE_GRID;
    
    int global_row = rand() % MAX_COORD;
    int global_col = rand() % MAX_COORD;
    
    // int division to get the grid coordinate
    int grid_row = global_row / SMALL_GRID;
    int grid_col = global_col / SMALL_GRID;
    
    // getting the module results gives us a number from 0 to 2
    int sensor_row = global_row % SMALL_GRID;
    int sensor_col = global_col % SMALL_GRID;
    
    // put fire on cell ('@')
    grid[grid_row][grid_col]->matrix[sensor_col][sensor_row] = '@';
    
    pthread_mutex_unlock(&grid_mutex);
}

int main(int argc, char const *argv[])
{
    srand(time(NULL));

    Sensor *sensors[WHOLE_GRID][WHOLE_GRID];
    initiateGrid(sensors);

    int fire_timer = 0;
    while (1)
    {
        if (fire_timer % 5 == 0)
            fire(sensors);

        system("clear");
        printSensorGrid(sensors);
        sleep(1);
        fire_timer += 5;
    }

    freeGrid(sensors);
    return 0;
}
