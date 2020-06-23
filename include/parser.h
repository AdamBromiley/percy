#ifndef PARSER_H
#define PARSER_H


#include <complex.h>
#include <stdint.h>


enum ParserErrorCode
{
    PARSER_NONE,
    PARSER_ERROR,
    PARSER_ERANGE,
    PARSER_EMIN,
    PARSER_EMAX,
    PARSER_EEND,
    PARSER_EBASE
};


int stringToULong(unsigned long *x, const char *nptr, unsigned long min, unsigned long max, char **endptr, int base);
int stringToUIntMax(uintmax_t *x, const char *nptr, uintmax_t min, uintmax_t max, char **endptr, int base);
int stringToDouble(double *x, const char *nptr, double min, double max, char **endptr);
int stringToImaginary(complex *z, char *nptr, complex min, complex max, char **endptr, int *type);
int stringToComplex(complex *z, char *nptr, complex min, complex max, char **endptr);


#endif