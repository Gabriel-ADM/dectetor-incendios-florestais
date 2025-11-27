#include <stdio.h>
#include <stdlib.h>

#define SMALL_GRID 3
#define WHOLE_GRID 10

void printCharGreenBg(char character)
{
    printf("\033[42m %c \033[m", character);
}

typedef struct Sensor
{
    int id;
    char matrix[SMALL_GRID][SMALL_GRID];
    int posX, posY;
} Sensor;

Sensor *initiateSensor(int id, int positionX, int positionY)
{
    Sensor *sensor = malloc(sizeof(Sensor));
    if (sensor == NULL) {
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

void printSensor(Sensor *sensor)
{
    for (int i = 0; i < SMALL_GRID; i++)
    {
        for (int j = 0; j < SMALL_GRID; j++)
        {
            printCharGreenBg(sensor->matrix[i][j]);
        }
        printf("\n");
    }
}

int main(int argc, char const *argv[])
{
    Sensor* first = initiateSensor(1,1,1);    
    printSensor(first);

    return 0;
}
