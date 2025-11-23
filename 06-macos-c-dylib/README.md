# Creating a C Dylib on macOS

## Background

A Dynamic Library `*.dylib` is a library that's loaded at runtime instead of at compile time.
They contain generic, unmodifiable code intended to be reused by many applications.

It is analogous to Windows `*.dll` file or a `*.so` file on Linux.

## Create a place to store the dylib and header file

Every `dylib` file will need a corresponding header (`.h`) file.

It is recommended to use `/usr/local/lib` for personal dylib files and then `/usr/local/include` for personal header files.

I will make a directory there to group mine:

```sh
$ sudo mkdir -p /usr/local/lib/standardloop/      # my dylibs will be put here
$ sudo mkdir -p /usr/local/include/standardloop/  # the corresponding header files will be placed here
```

## Example Dylib

I'll define a `dylib` with a simple function that will add two ints and return the sum.

### Header file


```sh
$ touch add.h
```
```c
#ifndef STANDARDLOOP_ADD_H
#define STANDARDLOOP_ADD_H

#define STANDARDLOOP_ADD_H_MAJOR_VERSION 0
#define STANDARDLOOP_ADD_H_MINOR_VERSION 0
#define STANDARDLOOP_ADD_H_PATCH_VERSION 1
#define STANDARDLOOP_ADD_H_VERSION "0.0.1"

int add(int, int);

#endif
```

### C file

```sh
$ touch add.c
```

```c
#include "add.h"

int add(int x, int y)
{
    return x + y;
}
```

### Compiling the Dynamic Library

```sh
$ gcc -Werror -Wextra -Wall -Wfree-nonheap-object \
    -std=c17 \
    add.c \
    -O3 \
    -dynamiclib \
    -current_version 0.0.1 \
    -compatibility_version 0.0.1 \
    -o libstandardloop-add.dylib
```

Notice the `-dynamiclib`, `current_version`, and `compatibility_version` flags.

We can then use `otool` to inspect it:

```sh
$ otool -L libstandardloop-add.dylib
libstandardloop-add.dylib:
        libstandardloop-add.dylib (compatibility version 0.0.1, current version 0.0.1)
        /usr/lib/libSystem.B.dylib (compatibility version 1.0.0, current version 1351.0.0)
```

### Moving the files

```sh
$ sudo cp libstandardloop-add.dylib /usr/local/lib/standardloop/
$ sudo cp add.h /usr/local/include/standardloop/
```

## Example Programming using the dylib

### C file
```c
#include <stdlib.h>
#include <stdio.h>

#include <standardloop/add.h>

int main()
{
    printf("%d\n", add(1, 2));
    return EXIT_SUCCESS;
}
```

Notice the import 😎

### Compiling

```sh
$ gcc -Werror -Wextra -Wall -Wfree-nonheap-object \
    -std=c17 \
    main.c  \
    -L/usr/local/lib/standardloop \
    -lstandardloop-add \
    -o main
```

notice the `-L` and `-l` flags used.

`-L` tells the compiler where to look for libraries.
`-l` tells the compiler what specific libraries to link.

### Running

```sh
$ DYLD_FALLBACK_LIBRARY_PATH=/usr/local/lib/standardloop/ ./main
3
```

Here we need to define a `DYLD_FALLBACK_LIBRARY_PATH` to tell the macOS dynamic linker where to find the `dylib`.

There is also `DYLD_LIBRARY_PATH`, but I don't recommend using this variable.

If it is not set, you will get an error like this:
```sh
$ ./main
dyld[77613]: Library not loaded: libstandardloop-add.dylib
...
```

I recommend adding this line to your `~/.zshenv`:
```sh
export DYLD_FALLBACK_LIBRARY_PATH="/usr/local/lib/standardloop"
```

So you don't have to worry about setting the variable when running the executable.


## Taskfile

Folder structure:

```sh
├── lib
│   ├── add.c
│   ├── add.h
│   └── Taskfile.yml
├── main.c
└── Taskfile.yml
```

### Compiling and moving the dylib (Taskfile in the lib directory)

```yaml
---
version: '3'

vars:
  CC: gcc
  CC_FLAGS: "-Werror -Wextra -Wall -Wfree-nonheap-object -std=c17"
  DYLIB_NAME: add
  DYLIB_NAME_FULL: "libstandardloop-{{.DYLIB_NAME}}.dylib"
  DYLIB_PATH: /usr/local/lib/standardloop/
  DYLIB_INCLUDE_PATH: /usr/local/include/standardloop/
  DYLIB_VERSION: 0.0.1
  SOURCE_FILES:
    - add.c

tasks:
  default:
    deps:
      - move-files

  build-release:
    cmds:
      - |
        {{.CC}} {{.CC_FLAGS}} \
        {{range .SOURCE_FILES}} {{.}} {{end}} \
        -O3 \
        -dynamiclib \
        -current_version {{.DYLIB_VERSION}} \
        -compatibility_version {{.DYLIB_VERSION}} \
        -o {{.DYLIB_NAME_FULL}}
    sources:
      - |
        {{range .SOURCE_FILES}} {{.}} {{end}}
    generates:
      - "{{.DYLIB_NAME_FULL}}"


  move-files:
    deps:
      - build-release
    cmds:
      - sudo -v
      - sudo cp {{.DYLIB_NAME_FULL}} {{.DYLIB_PATH}}
      - sudo cp {{.DYLIB_NAME}}.h {{.DYLIB_INCLUDE_PATH}}
    status:
      - otool -L {{.DYLIB_PATH}}{{.DYLIB_NAME_FULL}} | head -n 2 | tail -n 1 | grep "current version {{.DYLIB_VERSION}}"
      - cat {{.DYLIB_INCLUDE_PATH}}{{.DYLIB_NAME}}.h | grep "STANDARDLOOP_{{.DYLIB_NAME | upper}}_H_VERSION \"{{.DYLIB_VERSION}}\""

  clean:
    allow_failure: true
    cmds:
      - sudo -v
      - rm -f {{.DYLIB_NAME_FULL}}
      - sudo rm -f {{.DYLIB_PATH}}{{.DYLIB_NAME_FULL}}
      - sudo rm -f {{.DYLIB_INCLUDE_PATH}}{{.DYLIB_NAME}}.h
```

This file won't be run directly

### Compiling our test Program

```yaml
---
version: '3'

vars:
  CC: gcc
  CC_FLAGS: "-Werror -Wextra -Wall -Wfree-nonheap-object -std=c17"
  EXECUTABLE_NAME: main
  DYN_LIBS_USED_PATH: "-L/usr/local/lib/standardloop"
  DYN_LIBS_USED: "-lstandardloop-add"
  SOURCE_FILES:
    - main.c

tasks:
  default:
    deps:
      - run

  dependencies:
    cmds:
      - task --taskfile ./lib/Taskfile.yml

  build:
    deps:
      - dependencies
    cmds:
      - |
        {{.CC}} {{.CC_FLAGS}} \
        {{range .SOURCE_FILES}} {{.}} {{end}} \
        {{.DYN_LIBS_USED_PATH}} \
        {{.DYN_LIBS_USED}} \
        -o {{.EXECUTABLE_NAME}}
    sources:
      - |
        {{range .SOURCE_FILES}} {{.}} {{end}}
    generates:
      - "{{.EXECUTABLE_NAME}}"

  run:
    deps:
      - build
    cmds:
      - DYLD_FALLBACK_LIBRARY_PATH={{.DYLIB_PATH}} ./{{.EXECUTABLE_NAME}}
    vars:
      DYLIB_PATH: /usr/local/lib/standardloop/

  clean:
    allow_failure: true
    cmds:
      - rm -f {{.EXECUTABLE_NAME}}
      - task --taskfile ./lib/Taskfile.yml clean
```

```sh
$ task
task: [dependencies] task --taskfile ./lib/Taskfile.yml
task: [build-release] gcc -Werror -Wextra -Wall -Wfree-nonheap-object -std=c17 \
 add.c  \
-O3 \
-dynamiclib \
-current_version 0.0.1 \
-compatibility_version 0.0.1 \
-o libstandardloop-add.dylib

task: [move-files] sudo -v
task: [move-files] sudo cp libstandardloop-add.dylib /usr/local/lib/standardloop/
task: [move-files] sudo cp add.h /usr/local/include/standardloop/
task: [build] gcc -Werror -Wextra -Wall -Wfree-nonheap-object -std=c17 \
 main.c  \
-L/usr/local/lib/standardloop \
-lstandardloop-add \
-o main

task: [run] DYLD_FALLBACK_LIBRARY_PATH=/usr/local/lib/standardloop/ ./main
3
```

## Conclusion

This article is about creating your very own dynamic library for C on macOS.
This is how you can import with angle brackets instead of quotes.
We also looked at automating this whole demo with Taskfile.
