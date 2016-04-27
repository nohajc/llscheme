# llscheme
LLVM frontend for the Scheme language

This is a compiler and runtime for a minimal implementation of Scheme, written in C++ and using the LLVM framework.
It can translate Scheme source files (.scm) directly to native object files (.o).

## Installation

### Supported platforms
Linux

### Dependencies
  * gcc
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

## Quick start
The Makefile prepared in `test/lls_programs` scans all *.scm files in the directory, so you can add your own source file and compile it to executable by simply executing `make` or `make DEBUG=y` (it automatically invokes linker for you).

You can also try REPL (written in Scheme) to interactively compile and run entered code: `test/lls_programs/repl`


## Usage

### Compiler

```
$ schemec --help
USAGE: schemec [options] [input file]

Options:
             --help              Print usage and exit.
  -s <str>,  --string=<str>      String containing the input source code.
  -o <name>                      Output file name.
  -f <type>, --filetype=<type>   Output file type: asm, obj or null.
  -b <type>, --buildtype=<type>  Build type: exec or lib.
  -O <num>                       Optimization level: -O0, -O1, -O2, -O3.
```

Default output file type is obj (.o), default build type is exec (object file with main function).

#### Compile input file
```
$ schemec my_exec.scm   # builds my_exec.o with main function, you can provide a different output name using -o
$ clang my_exec.o -o my_exec

$ schemec my_lib.scm -b lib   # builds my_lib.o without main, compiles to position independent code (-fPIC)
$ clang my_exec.o my_lib.o -o my_exec_with_lib
```

#### Compile input string
```
$ schemec -s "(display \"sometext\")" -o my_exec.o   # if no -o provided, prints the binary to stdout
```

## Features

```scheme
; Definitions
(define foo "bar") ; variable
(define (func x y z) (list x y z)) ; function

; Atoms
"string"
symbol
7 ; integer
3.14 ; float
#t ; true
#f ; false
null

; Expressions
(func 1 2 3) ; function call

(quote 1 2 3)
; or
'(1 2 3) ; constant data

(lambda (x) (- x)) ; anonymous function

(if (> 10 3) "ten is greater" "impossible") ; if/then/else

(let ((a 1) (b 2) (c 3)) ; declare local variables
  (define d (* b c))     ; use them
  (display (+ a b c d))) ; multiple expressions allowed
  
(and 2 4 #f) ; logical and - returns false
(or #f 2 4) ; logical or - returns 2

(define (foo a b)
  (lambda (bar c)
    (lambda (x) (+ a b c x)))) ; nested closures


; Built-in functions
; also see the example programs (test/lls_programs) 
; and library functions written in Scheme (src/runtime/scmlib.scm)

;   Arithmetic functions: +, -, *, /
(- 3) ; -3
(- 3 2) ; 1
(+ 0 9 8 7) ; arbitrary number of arguments

;   Arithmetic operators: =, <, >, <=, >=

;   Predicates: null?, zero?, eof-object?
;   Logical: not

;   Type and value comparison: equal?
(equal? "a" "a") ; #t
(equal? '(1 2) '(3 4)) ; #f

;   Print operations
(display "foo") ; also displays other types
(displayln "bar") ; display with \n


;   List operations
(cons 'a (cons 'b (cons 'c null))) ; constructs lists
(list 'a 'b 'c) ; constructs lists more conveniently

(car lst) ; first element of the list
(cdr lst) ; rest of the list (from the second element)
(list-ref lst nth) ; nth element of list

(apply + (list 1 2 3)) ; applies list of arguments to a function

(length lst) ; number of elements in lst

(define lst '(1 2 3 4 5))
(map (lambda (x) (+ x x)) lst) ; map function to elements of list (returns new list)

(append lst1 lst2) ; append lists

(filter fn lst) ; filter elements of lst for which (fn elem) is #t

(zip '(1 2 3) '(a b c)) ; => '((1 a) (2 b) (3 c))

(member elem lst) ; is elem member of lst?

(remove-duplicates lst)


;   User input
(read) ; read data (atoms or lists) from stdin


;   Code evaluation
(define ns (make-base-namespace)) ; create namespace (environment for eval, populated by library functions)
(current-namespace ns) ; set current namespace
(current-namespace) ; get current namespace

(eval '(displayln "Eval works!") ns) ; eval in the namespace


;   String operations
(string->symbol "str") ; converts string to symbol
(string=? "a" "b") ; compares two strings
(string-append "abraka" "dabra") ; => "abrakadabra"
(string-replace "abraka" "a" "o") ; => "obroko"
(string-split "abraka dabra") ; => ("abraka" "dabra")


;   File operations
(define fhandle (open-input-file "filename"))
(read-line fhandle) ; returns string or eof, behaves like a stream
(close-input-port fhandle)

```

