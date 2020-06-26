# Percy Parser

Percy Parser is a set of wrappers around the standard C library `strtoX()` functions designed to make error-handling easier and cleaner.

## Features
- Wrappers around many standard `strtoX()` functions
- Complex number parsing support
- Memory value parsing (with or without units)

## Installation
`make` from the project's root directory to build the `libpercy.so` shared object.

For use with programs, refer to the following example:

### Example
Consider the project root as `libpercy` and the library is being used by `foo.c`:

1. Use `#include "parser.h"` in `foo.c`
2. Compile with the necessary include path; link with the correct dynamic library linkage; and run:
    ```
    ~$ gcc -c foo.c -Ilibpercy/include -o foo.o

    ~$ gcc foo.o -Llibpercy -Wl,-rpath=libpercy -lpercy -o foo

    ~$ ./foo
    ```

## Usage
Every command has the following signature:

```C
ParseErr stringToType(type *x, char *nptr, type min, type max, char **endptr, /* additional parameters */);
```

The value parsed from the string pointed to by `nptr` will be placed in `*x`. If outside `min`/`max` (or under/overflows the data type), an exception will be thrown.

The position of `*endptr` will be at the end (the first non-value character) of the parsed value, or in an undefined location on error.

Additional parameters may by required by some functions.

### Additional Parameters

| Parameter         | Usage |
| :---------------- | :---- |
| `int base`        | Specify the radix of the input from `2` to `32`. The `NumBase` type defines several commonly-used bases that can be used instead of a literal: `BASE_BIN`, `BASE_OCT`, `BASE_DEC`, `BASE_HEX`, and `BASE_32` |
| `ComplexPt *type` | `stringToComplexPart()` stores what part of a complex number it parsed in this variable. It will take on the value `COMPLEX_REAL` or `COMPLEX_IMAGINARY` |
| `int magnitude`   | For memory-value parsing, it may be more user-friendly to take, for example, a MB input as default rather than just bytes. This value will set the assumed magnitude for a suffix-less memory-value input. A set of common magnitudes, `MEM_B`, `MEM_KB`, `MEM_MB`, ... , `MEM_YB` are defined by the `MemMag` type. `MEM_B` or `0` should be used for standard byte-magnitude input |

### Error Handling
The return type of every function is `ParseErr`, an integer type defining a variety of error codes for the library. Success will always return `0` and errors (including an indicator for suffixed, un-parsable text) will return non-zero.

| Return          | Description |
| :-------------- | :---------- |
| `PARSE_SUCCESS` | Success (will always `= 0`) |
| `PARSE_EERR`    | Unknown error - often because of text mixed in with integers |
| `PARSE_ERANGE`  | Out of range of the data type, resulting in under/overflow |
| `PARSE_EMIN`    | Value is less than the mininum function argument |
| `PARSE_EMAX`    | Value is greater than the maximum function argument |
| `PARSE_EEND`    | Success but extra, unparsable data remains at the end of the string. `*endptr` will point to the first non-value character |
| `PARSE_EBASE`   | Invalid radix specified in the function argument |
| `PARSE_EFORM`   | Invalid format of the inputted string (if not caught as `PARSE_EERR`) |

## Parsing Functions
### Integers

```C
// Parse `unsigned long int`
stringToULong(unsigned long *x, /* ... */, int base);

// Parse `uintmax_t`
stringToUIntMax(uintmax_t *x, /* ... */, int base);
```

### Floating-points
A floating-point is valid input in any of the C-specified formats. This includes normal `0.123` and hexadecimal `0x8.9AB` numbers, along with their respective exponential (`e` and `p`) extensions and optional sign prefix (with any amount of preceding whitespace.)

```C
// Parse `double`
stringToDouble(double *x, /* ... */);
```

### Complex Numbers
Similarly, complex numbers in the form `a + bi` can be parsed, where `a` and `b` are `double` values parsed in the aforementioned manner.

Either part is optional and can be in any order, and the operator can be either sign and used in conjunction with a second sign on the following value. The imaginary unit must be included as `i` or `I` on the imaginary part and does not require a coefficient; if there is a coefficient, all parts (including an exponential token) must be preceding it.

The values are put into a `complex` variable, which is zero'ed before parsing to allow omitted parts to be parsed as `0.0`. If this is unwanted (so if only one complex part is to be inputted) behaviour, the `stringToComplexPart()` function should be used so as to preserve the other complex part in the variable.

```C
// Parse a real or imaginary part of `complex`
stringToComplexPart(complex *z, /* ... */, ComplexPt *type);

// Parse `complex`
stringToComplex(complex *z, /* ... */);
```

For minimum/maximum `complex` values, the following global constants are initialised:

```C
/* Minimum/maximum possible complex values */
extern const complex CMPLX_MIN = -(DBL_MAX) - DBL_MAX * I;
extern const complex CMPLX_MAX = DBL_MAX + DBL_MAX * I;
```

### Memory Values
For user input of `size_t`, the following function is provided. It allows an optional suffix of `B`, `kB`, `MB`, `GB`, `TB`, `PT`, `EB`, `ZB`, `YT` to denote a magnitude ranging from `e0` to `e24`. The input is first parsed as a `double`, so any of the standard `double` formats are valid. The number is then scaled up according to the unit, then converted to a `size_t` (with decimal truncation if the scaled result is not a whole number).

If no unit is provided, the magnitude is assumed to be that of the `int magnitude` argument. A set of common magnitudes, `MEM_B`, `MEM_KB`, `MEM_MB`, ... , `MEM_YB` are defined by the `MemMag` type for this purpose, but an integer argument will suffice (or for non-standard magnitudes). For standard byte input, this should be `0` (or `MEM_B`.)

```C
// Parse `size_t` with optional memory unit suffix
stringToMemory(size_t *bytes, /* ... */, int magnitude);
```

### Demonstration
Look in [test/percy_demo.c](test/percy_demo.c) for a practical use of the library and a subset of its functions. Run `make demo` from the project's root to compile the demonstration script, and run with `./percy_demo [OPTIONS...]`