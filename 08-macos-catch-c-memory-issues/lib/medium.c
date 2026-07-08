#include <stdlib.h>
#include <stdio.h>

#include "medium.h"

void GenerateLeak()
{
    printf("testing forget to free from dylib function GenerateLeak\n");
    const int num = 1000;
    int *example = malloc(sizeof(int) * num);
    if (example == NULL)
    {
        return;
    }
    for (int i = 0; i < num; i++)
    {
        example[i] = 40; // random number
    }
}
