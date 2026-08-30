#include <stdio.h>

int main(void)
{
    // Standard colors (0-7)
    printf("=== Standard (0-7) ===\n");
    for (int i = 0; i <= 7; i++)
    {
        printf("\033[38;5;%dmColor %3d\033[0m  ", i, i);
    }
    printf("\n\n");

    // Bright colors (8-15)
    printf("=== Bright (8-15) ===\n");
    for (int i = 8; i <= 15; i++)
    {
        printf("\033[38;5;%dmColor %3d\033[0m  ", i, i);
    }
    printf("\n\n");

    // 216 color cube (16-231)
    printf("=== 216 Color Cube (16-231) ===\n");
    for (int i = 16; i <= 231; i++)
    {
        printf("\033[38;5;%dm%3d \033[0m", i, i);
        if ((i - 15) % 12 == 0)
        {
            printf("\n");
        }
    }
    printf("\n");

    // Grayscale ramp (232-255)
    printf("=== Grayscale (232-255) ===\n");
    for (int i = 232; i <= 255; i++)
    {
        printf("\033[38;5;%dm%3d \033[0m", i, i);
    }
    printf("\n");

    return 0;
}
