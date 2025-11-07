#include <stdio.h>
#include <stdlib.h>

#define SMALL_GRID 3
#define WHOLE_GRID 30

void printGreenBg(char *text)
{
    printf("\033[42m %s \033[m", text);
}

int main(int argc, char const *argv[])
{
    for (int i = 0; i < SMALL_GRID; i++)
    {
        for (int j = 0; j < SMALL_GRID; j++)
        {
            if (i == SMALL_GRID / 2 && j == SMALL_GRID / 2)
            {
                printGreenBg("T");
            }
            else
            {
                printGreenBg("-");
            }
        }
        printf("\n");
    }

    return 0;
}
