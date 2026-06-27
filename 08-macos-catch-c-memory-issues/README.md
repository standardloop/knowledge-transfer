# Tools on macOS for memory leaks in C code

## clang flags

https://clang.llvm.org/docs/AddressSanitizer.html#usage

```sh
    -fsanitize=address
```

To get nicer stack traces in error messages add `-fno-omit-frame-pointer`. To get perfect stack traces you may need to disable inlining (just use `-O1`) and tail call elimination (`-fno-optimize-sibling-calls`).

## leaks

### Normal Usage

```sh
$ leaks --atExit -- ./main
```

### Usage with Dynamic libraries

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
