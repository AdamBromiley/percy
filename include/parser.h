#ifndef PARSER_H
#define PARSER_H


#include <complex.h>
#include <stddef.h>
#include <stdint.h>


enum PercyParserError
{
    PARSE_SUCCESS = 0,
    PARSE_EERR,
    PARSE_ERANGE,
    PARSE_EMIN,
    PARSE_EMAX,
    PARSE_EEND,
    PARSE_EBASE,
    PARSE_EFORM
};

enum PercyNumberBase
{
    BASE_BIN = 2,
    BASE_TER = 3,
    BASE_OCT = 8,
    BASE_DEC = 10,
    BASE_HEX = 16,
    BASE_32 = 32
};

enum PercyComplexPart
{
    COMPLEX_NONE,
    COMPLEX_REAL,
    COMPLEX_IMAGINARY
};

enum PercyMemoryMagnitude
{
    MEM_B = 0,
    MEM_KB = 3,
    MEM_MB = 6,
    MEM_GB = 9,
    MEM_TB = 12,
    MEM_PB = 15,
    MEM_EB = 18,
    MEM_ZB = 21,
    MEM_YB = 24
};


typedef enum PercyParserError ParseErr;
typedef enum PercyNumberBase NumBase;
typedef enum PercyComplexPart ComplexPt;
typedef enum PercyMemoryMagnitude MemMag;


extern const complex CMPLX_MIN;
extern const complex CMPLX_MAX;


ParseErr stringToULong(unsigned long *x, char *nptr, unsigned long min, unsigned long max, char **endptr, int base);
ParseErr stringToUIntMax(uintmax_t *x, char *nptr, uintmax_t min, uintmax_t max, char **endptr, int base);
ParseErr stringToDouble(double *x, char *nptr, double min, double max, char **endptr);
ParseErr stringToComplexPart(complex *z, char *nptr, complex min, complex max, char **endptr, ComplexPt *type);
ParseErr stringToComplex(complex *z, char *nptr, complex min, complex max, char **endptr);
ParseErr stringToMemory(size_t *bytes, char *nptr, size_t min, size_t max, char **endptr, int magnitude);

size_t strncpyGraph(char *dest, const char *src, size_t n);


#endif