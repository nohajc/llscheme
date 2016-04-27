# llscheme
LLVM frontend for the Scheme language

This is a compiler and runtime for a minimal implementation of Scheme, written in C++ and using the LLVM framework.
It can translate Scheme source files (.scm) directly to native object files (.o).

## Installation

### Dependencies
  * llvm
  * clang
  * libgc (Boehm garbage collector)
  * cmake
  * boost filesystem

### Build the compiler and runtime library

```
$ git clone https://github.com/nohajc/llscheme.git
$ cd llscheme/build
$ ./configure
$ make
```

For debug build with address sanitizer, run `./configure --enable-debug`.

### Build the example programs
```
$ cd llscheme/test/lls_programs
$ make
```

Or `make DEBUG=y` for the debug version.

The compiler executable and the library will be located in `bin/Release` or `bin/Debug`.

If you want to run compiled programs outside of the `lls_programs` directory, install `libllscmrt.so` to a standard location or use `LD_LIBRARY_PATH`.

## Usage

## Features
