#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <standardloop/medium.h>

int main(int argc, char **argv)
{
    printf("starting program: %s with argc %d\n", argv[0], argc);
    GenerateLeak();

    sleep(1);
    return EXIT_SUCCESS;
}
