#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

enum Mode
{
    UseAfterFree = 0,
    OutOfBounds = 1,
    ForgetToFree = 2,
    // UseUninitialized = 3,
};

const char *option_message = "UseAfterFree = 0\nOutOfBounds = 1\nForgetToFree = 2\n";

void useAfterFree()
{
    // -fsanitize=address
    printf("testing use after free\n");
    const int num = 1000;
    int *example = malloc(sizeof(int) * num);
    free(example);
    printf("%d\n", example[0]);
}

void outOfBounds()
{
    // -fsanitize=address
    printf("testing out of bounds access\n");
    const int num = 1000;
    int *example = malloc(sizeof(int) * num);
    printf("%d\n", example[100000]);
    free(example);
}

void forgetToFree()
{
    // leaks
    printf("testing forget to free\n");
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
    // example = NULL;
    // sleep(20);
}

int main(int argc, char **argv)
{
    printf("starting program: %s with argc %d\n", argv[0], argc);

    if (argc < 2)
    {
        printf("Please input an option\n");
        printf("%s", option_message);
        return EXIT_FAILURE;
    }

    enum Mode option = atoi(argv[1]);

    if (option == UseAfterFree)
    {
        useAfterFree();
    }
    else if (option == OutOfBounds)
    {
        outOfBounds();
    }
    else if (option == ForgetToFree)
    {
        forgetToFree();
    }
    // else if (option == UseUninitialized)
    // {
    //     useUninitialized();
    // }
    else
    {
        printf("invalid option\n");
        printf("%s", option_message);
        return EXIT_FAILURE;
    }
    sleep(1);
    return EXIT_SUCCESS;
}
