#include <stdio.h>
#include <stdlib.h>

enum Mode
{
    UseAfterFree = 0,
    OutOfBounds = 1,
    ForgetToFree = 2,
    UseUninitialized = 3,
};

void useAfterFree()
{
    // -fsanitize=address
    printf("testing use after free\n");
    const int num = 1000;
    int *example = calloc(num, sizeof(int));
    free(example);
    printf("%d\n", example[0]);
}

void outOfBounds()
{
    // -fsanitize=address
    printf("testing out of bounds access\n");
    const int num = 1000;
    int *example = calloc(num, sizeof(int));
    printf("%d\n", example[100000]);
    free(example);
}

void forgetToFree()
{
    // leaks
    printf("testing forget to free\n");
    const int num = 1000;
    int *example = calloc(num, sizeof(int));
    (void)example;
}

void useUninitialized()
{
    // MallocPreScribble
    printf("testing accessing uninitialized\n");
    const int num = 1000;
    int *example = malloc(sizeof(int) * num);
    printf("%d\n", example[0]);
    free(example);
}

int main(int argc, char **argv)
{
    printf("starting program: %s with argc %d\n", argv[0], argc);

    if (argc < 2)
    {
        printf("Please input an option\n");
        printf("UseAfterFree = 0\nOutOfBounds = 1\nForgetToFree = 2\nUseUninitialized = 3\n");
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
    else if (option == UseUninitialized)
    {
        useUninitialized();
    }
    else
    {
        printf("invalid option\n");
        printf("UseAfterFree = 0\nOutOfBounds = 1\nForgetToFree = 2\nUseUninitialized = 3\n");
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}
