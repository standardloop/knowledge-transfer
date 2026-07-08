# macOS Tools for Catching Memory Issues For a C Program

- [https://github.com/standardloop/knowledge-transfer/tree/main/08-macos-catch-c-memory-issues](https://github.com/standardloop/knowledge-transfer/tree/main/08-macos-catch-c-memory-issues)


## Tools

In this article I will be exploring `clang` flags and macOS `leaks` with an example program to show what they are capable of.

On linux, people will use `valgrind` but unfortunately this is not an option on macOS.

### `clang` Flags Intro

https://clang.llvm.org/docs/AddressSanitizer.html#usage

```sh
    -fsanitize=address
```

To get nicer stack traces in error messages add `-fno-omit-frame-pointer`. To get perfect stack traces you may need to disable inlining (just use `-O1`) and tail call elimination (`-fno-optimize-sibling-calls`).

### `leaks` Intro

You can run `man leaks` to get more information

#### Normal Usage

```sh
$ leaks --atExit -- ./main
```

## Demo Program

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

Use my Taskfile to compile everything needed
```sh
$ task
```

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

No errors, the code just runs as if there are no issues.

#### `clang` flags

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

Note:

```txt
Can't examine target process's malloc zone asan_0x10525f0d0, so memory analysis will be incomplete or incorrect.
Reason: target process is using Address Sanitizer which doesn't work with memory analysis tools
```

You cannot use `leaks` on a binary that was compiled with address sanitizer.

We will use the binary compiled without the address sanitizer

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

You can see, `leaks` has identified a leak!

#### Using Leaks with a Dynamic Library

You can see my other article for making dynamic libraries on macOS:
- https://medium.com/@standardloop/creating-a-c-library-on-macos-angled-brackets-7a972863ed11
- https://github.com/standardloop/knowledge-transfer/tree/main/06-macos-c-dylib

##### Example Program

Dylib source
```c
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
```


Main source
```c

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
```

##### Compiling
```sh
$ clang -O0 -Werror -Wextra -Wall -Wfree-nonheap-object -std=c17 \
    main-with-lib.c  \
    -L/usr/local/lib/standardloop \
    -lstandardloop-medium \
    -o main-with-lib

$ leaks --atExit -- ./main-with-lib
dyld[78616]: Library not loaded: libstandardloop-medium.dylib
  Referenced from: <6783766B-C4F2-4361-8667-189D309D3514> /Users/redacted/Code/knowledge-transfer/08-macos-catch-c-memory-issues/main-with-lib
  Reason: tried: 'libstandardloop-medium.dylib' (no such file), '/System/Volumes/Preboot/Cryptexes/OSlibstandardloop-medium.dylib' (no such file), 'libstandardloop-medium.dylib' (no such file), '/Users/redacted/Code/knowledge-transfer/08-macos-catch-c-memory-issues/libstandardloop-medium.dylib' (no such file), '/System/Volumes/Preboot/Cryptexes/OS/Users/redacted/Code/knowledge-transfer/08-macos-catch-c-memory-issues/libstandardloop-medium.dylib' (no such file), '/Users/redacted/Code/knowledge-transfer/08-macos-catch-c-memory-issues/libstandardloop-medium.dylib' (no such file)
```

Even though I have my `DYLD_FALLBACK_LIBRARY_PATH` variable set in my `~/.zshenv`, `leaks` can't pick it up.

##### My Quick Fix

Just copy the dylib to the local directory and then run leaks

```sh
$ cp /usr/local/lib/standardloop/libstandardloop-medium.dylib ./
$ leaks --atExit -- ./main-with-lib
$ rm -f libstandardloop-medium.dylib
```

Output

```sh
$ leaks --atExit -- ./main-with-lib
main-with-lib(79483) MallocStackLogging: could not tag MSL-related memory as no_footprint, so those pages will be included in process footprint - No such file or directory (2)
main-with-lib(79483) MallocStackLogging: recording malloc (and VM allocation) stacks using lite mode
starting program: ./main-with-lib with argc 1
testing forget to free from dylib function GenerateLeak
Process 79483 is not debuggable. Due to security restrictions, leaks can only show or save contents of readonly memory of restricted processes.

Process:         main-with-lib [79483]
Path:            /Users/USER/*/main-with-lib
Load Address:    0x104cb4000
Identifier:      main-with-lib
Version:         0
Code Type:       ARM64
Platform:        macOS
Parent Process:  leaks [79482]
Target Type:     live task

Date/Time:       2026-07-08 15:23:48.362 +0800
Launch Time:     2026-07-08 15:23:47.342 +0800
OS Version:      macOS 26.5.1 (25F80)
Report Version:  7
Analysis Tool:   /usr/bin/leaks

Physical footprint:         2096K
Physical footprint (peak):  2096K
Idle exit:                  untracked
----

leaks Report Version: 4.0, multi-line stacks
Process 79483: 190 nodes malloced for 20 KB
Process 79483: 1 leak for 4096 total leaked bytes.

STACK OF 1 INSTANCE OF 'ROOT LEAK: <malloc in GenerateLeak>':
3   dyld                                  0x18162be00 start + 6992
2   main-with-lib                         0x104cb44e8 main + 72
1   libstandardloop-medium.dylib          0x104ccc440 GenerateLeak + 40
0   libsystem_malloc.dylib                0x181816178 _malloc_zone_malloc_instrumented_or_legacy + 152
====
    1 (4.00K) ROOT LEAK: <malloc in GenerateLeak 0xc20c44000> [4096]

...
```

We can see the leak is coming from `GenerateLeak` which is from our dynamic Library.

## Extra

- https://developer.apple.com/library/archive/documentation/System/Conceptual/ManPages_iPhoneOS/man3/malloc.3.html


```txt
MallocPreScribble:

If set, fill memory that has been allocated
with 0xaa bytes.  This increases the likelihood
that a program making assumptions about
the contents of freshly allocated memory will fail.
```

```txt
MallocScribble:

If set, fill memory that has been deallocated with 0x55 bytes.
This increases the likelihood that a program will fail due to
accessing memory that is no longer allocated.
```

Set like this

```sh
MallocScribble=1
MallocPreScribble=1
```

I haven't found a use for these quite yet, but if I do, I will make another article!

## Summary

Today we learned about `clang`'s address sanitizer and macOS's `leaks` tool.

We created an example program to show what they are all capable of.

We also looked into using `leaks` when your program relies on a dynamic library.
