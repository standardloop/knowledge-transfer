# Tools on macOS for memory leaks in C code

## Tools
### clang flags

https://clang.llvm.org/docs/AddressSanitizer.html#usage

```sh
    -fsanitize=address
```

To get nicer stack traces in error messages add `-fno-omit-frame-pointer`. To get perfect stack traces you may need to disable inlining (just use `-O1`) and tail call elimination (`-fno-optimize-sibling-calls`).

### leaks

#### Normal Usage

```sh
$ leaks --atExit -- ./main
```

#### Usage with Dynamic libraries


## Example Program

```c
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

enum Mode
{
    UseAfterFree = 0,
    OutOfBounds = 1,
    ForgetToFree = 2,
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
    else
    {
        printf("invalid option\n");
        printf("%s", option_message);
        return EXIT_FAILURE;
    }
    sleep(1);
    return EXIT_SUCCESS;
}
```

### Running

#### No Tools

```sh
$ starting program: ./main with argc 2
testing use after free
-1109987166

$ starting program: ./main with argc 2
testing out of bounds access
0

$ starting program: ./main with argc 2
testing forget to free
```

No errors, the code just runs lol

#### clang flags

```sh
$ ./main-sanitize 0
starting program: ./main-sanitize with argc 2
testing use after free
...
SUMMARY: AddressSanitizer: heap-use-after-free (main-sanitize:arm64+0x100000890) in useAfterFree+0x90
Shadow bytes around the buggy address:
...
==22319==ABORTING
zsh: abort      ./main-sanitize 0

$ ./main-sanitize 1
starting program: ./main-sanitize with argc 2
testing out of bounds access
AddressSanitizer:DEADLYSIGNAL
=================================================================
==22404==ERROR: AddressSanitizer: BUS on unknown address (pc 0x000104df8968 bp 0x00016b0063e0 sp 0x00016b0063a0 T0)
==22404==The signal is caused by a READ memory access.
==22404==Hint: this fault was caused by a dereference of a high value address (see register values below).  Disassemble the provided pc to learn which register was used.
    #0 0x000104df8968 in outOfBounds+0xa8 (main-sanitize:arm64+0x100000968)
    #1 0x000104df8aec in main+0x11c (main-sanitize:arm64+0x100000aec)
    #2 0x00018162bdfc in start+0x1b4c (dyld:arm64e+0x1fdfc)

$ ./main-sanitize 2
starting program: ./main-sanitize with argc 2
testing forget to free

# for me, there is no error here
```

### leaks

MallocStackLogging=1

note:

```txt
Can't examine target process's malloc zone asan_0x10525f0d0, so memory analysis will be incomplete or incorrect.
Reason: target process is using Address Sanitizer which doesn't work with memory analysis tools
```

```sh
leaks --atExit -- ./main 2
...
starting program: ./main with argc 2
testing forget to free
...
leaks Report Version: 4.0, multi-line stacks
Process 24632: 190 nodes malloced for 20 KB
Process 24632: 1 leak for 4096 total leaked bytes.

STACK OF 1 INSTANCE OF 'ROOT LEAK: <malloc in forgetToFree>':
3   dyld                                  0x18162be00 start + 6992
2   main                                  0x10028470c main + 208
1   main                                  0x1002845e0 forgetToFree + 40
0   libsystem_malloc.dylib                0x181816178 _malloc_zone_malloc_instrumented_or_legacy + 152
====
    1 (4.00K) ROOT LEAK: <malloc in forgetToFree 0xbf9008000> [4096]
...
```

## Extra

https://developer.apple.com/library/archive/documentation/System/Conceptual/ManPages_iPhoneOS/man3/malloc.3.html


```txt
MallocPreScribble:

If set, fill memory that has been allocated
with 0xaa bytes.  This increases the likelihood
that a program making assumptions about
the contents of freshly allocated memory will fail.

```txt
MallocScribble:
If set, fill memory that has been deallocated with 0x55 bytes.
This increases the likelihood that a program will fail due to
accessing memory that is no longer allocated.
```

MallocScribble=1
MallocPreScribble=1
