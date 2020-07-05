# Percy Parser

Percy Parser is a set of wrappers around the standard C library `strtoX()` functions designed to make error-handling easier and cleaner.

## Features
- Wrappers around many standard `strtoX()` functions
- Complex number parsing support
- Memory value parsing (with or without units)

## Dependencies
The following dependencies must be installed to system:
- The [GNU Multiple Precision Arithmetic Library](https://gmplib.org/) (GMP), version 5.0.0 or later
- The [GNU Multiple Precision Floating-Point Reliable Library](https://www.mpfr.org/) (MPFR), version 3.0.0 or later
- The [GNU Multiple Precision Complex Library](http://www.multiprecision.org/mpc/home.html) (MPC)

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

### Arbitrary-precision Numbers
Percy Parser also supports arbitrary-precision number parsing via the GNU Multiple Precision Floating-Point Reliable Library (MPFR) and it's complex extension, the GNU Multiple Precision Complex Library (MPC).

These extensions provide the following general function signature:
```C
ParseErr stringToType(type *x, char *nptr, type *min, type *max, char **endptr, /* additional parameters */);
```

Unlike the standard C type parsing, arbitrary-precision parsing requries the minimum and maximum values to be pointers to allow for the value `NULL` to specify no limit (because there is no `MPFR_MAX`, for example.)

`x` must be initialised first with its type's respective initialisation function (for example, for `mpfr_t`, the function `mpfr_init2()` or similar) - it hence must also be freed at some point after.

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

// Parse `long double`
stringToDoubleL(long double *x, /* ... */);

// Parse `mpfr_t`
stringToMPFR(mpfr_t *x, )
```

### Complex Numbers
Similarly, complex numbers in the form `a + bi` can be parsed, where `a` and `b` are `double` values parsed in the aforementioned manner.

Either part is optional and can be in any order, and the operator can be either sign and used in conjunction with a second sign on the following value. The imaginary unit must be included as `i` or `I` on the imaginary part and does not require a coefficient; if there is a coefficient, all parts (including an exponential token) must be preceding it.

The values are put into a `complex` variable, which is zero'ed before parsing to allow omitted parts to be parsed as `0.0`. If this is unwanted (so if only one complex part is to be inputted) behaviour, the `stringToComplexPart()` function should be used so as to preserve the other complex part in the variable.

```C
// Parse a real or imaginary part of `complex`
stringToComplexPart(complex *z, /* ... */, ComplexPt *type);

// Parse a real or imaginary part of `long double complex`
stringToComplexPartL(long double complex *z, /* ... */, ComplexPt *type);

// Parse `complex`
stringToComplex(complex *z, /* ... */);

// Parse `long double complex`
stringToComplexL(long double complex *z, /* ... */);
```

For minimum/maximum `complex` values, the following global constants are initialised:

```C
/* Minimum/maximum possible complex values */
extern const complex CMPLX_MIN = -(DBL_MAX) - DBL_MAX * I;
extern const complex CMPLX_MAX = DBL_MAX + DBL_MAX * I;

/* Minimum/maximum possible long double complex values */
extern const long double complex CMPLX_MIN = -(LDBL_MAX) - LDBL_MAX * I;
extern const long double complex CMPLX_MAX = LDBL_MAX + LDBL_MAX * I;
```

### Memory Values
For user input of `size_t`, the following function is provided. It allows an optional suffix of `B`, `kB`, `MB`, `GB`, `TB`, `PT`, `EB`, `ZB`, `YT` to denote a magnitude ranging from `e0` to `e24`. The input is first parsed as a `double`, so any of the standard `double` formats are valid. The number is then scaled up according to the unit, then converted to a `size_t` (with decimal truncation if the scaled result is not a whole number).

If no unit is provided, the magnitude is assumed to be that of the `int magnitude` argument. A set of common magnitudes, `MEM_B`, `MEM_KB`, `MEM_MB`, ... , `MEM_YB` are defined by the `MemMag` type for this purpose, but an integer argument will suffice (or for non-standard magnitudes). For standard byte input, this should be `0` (or `MEM_B`.)

```C
// Parse `size_t` with optional memory unit suffix
stringToMemory(size_t *bytes, /* ... */, int magnitude);
```

### Arbitrary-precision
Each function works the same as its standard counterpart and accepts the same syntax. Likewise, errors returned are the same.

The number variable must be initialised beforehand according to its respective library's initialisation sequence, otherwise what is returned is undefined.

Each function takes the number radix, `int base`, as an argument. This must be in the range `2` to `62` - a provided `enum` can be used to add verbosity to code for any common number base, as explained in [Additional Parameters](#additional-parameters). The special base of `0` will tell the parser to automatically determine the input type, dependent on the value's prefix (`0x`, `0b`, or none).

#### Floating-point
For input of an `mpfr_t` floating-point, the base and rounding mode must be specified.

`rnd` is the rounding mode for the floating-point, specified with one of the definitions provided in the MPFR library (of the form `MPFR_RNDx`.)
```C
// Parse `mpfr_t`
ParseErr stringToMPFR(mpfr_t *x, /* ... */, int base, mpfr_rnd_t rnd);
```

#### Complex
Input of an `mpc_t` type requires the same formatting as the [standard complex type](#complex-numbers).

The precision (number of significant bits) of the value must be specified, which is a positive integer of type `mpfr_prec_t`, defined in the MPFR library.

The rounding mode should be a definition from the MPC library, which should be of the form `MPC_RNDxy`.

```C
// Parse a real or imaginary part of `mpc_t`
ParseErr stringToComplexPartMPC(mpc_t *z, /* ... */, int base, mpfr_prec_t prec, mpc_rnd_t rnd, ComplexPt *type);

// Parse `mpc_t`
ParseErr stringToComplexMPC(mpc_t *z, /* ... */, int base, mpfr_prec_t prec, mpc_rnd_t rnd);
```

### Demonstration
Look in [test/percy_demo.c](test/percy_demo.c) for a practical use of the library and a subset of its functions. Run `make demo` from the project's root to compile the demonstration script, and run with `./percy_demo [OPTIONS...]`