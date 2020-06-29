#include "parser.h"

#include <complex.h>
#include <ctype.h>
#include <errno.h>
#include <float.h>
#include <inttypes.h>
#include <limits.h>
#include <math.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>


/* Minimum/maximum possible complex values */
const complex CMPLX_MIN = -(DBL_MAX) - DBL_MAX * I;
const complex CMPLX_MAX = DBL_MAX + DBL_MAX * I;


/* Symbol to denote the imaginary unit (case-insensitive) */
static const char IMAGINARY_UNIT = 'i';


static int parseMemoryUnit(char *str, char **endptr);
static int parseSign(char *c, char **endptr);
static ComplexPt parseImaginaryUnit(char *c, char **endptr);


/* Convert string to unsigned long and handle errors */
ParseErr stringToULong(unsigned long *x, char *nptr, unsigned long min, unsigned long max, char **endptr, int base)
{
    char sign;

    *endptr = nptr;

    if ((base < 2 && base != 0) || base > 36)
        return PARSE_EBASE;

    /* Get pointer to start of number */
    while (isspace(**endptr))
        ++(*endptr);

    sign = **endptr;

    nptr = *endptr;
    errno = 0;
    *x = strtoul(nptr, endptr, base);
    
    /* Conversion check */
    if (*endptr == nptr || errno == EINVAL)
        return PARSE_EERR;
    
    /* Range checks */
    if (errno == ERANGE)
        return PARSE_ERANGE;
    else if (*x < min)
        return PARSE_EMIN;
    else if (*x > max)
        return PARSE_EMAX;
    else if (sign == '-' && *x != 0)
        return PARSE_EMIN;

    /* If more characters in string */
    return (**endptr == '\0') ? PARSE_SUCCESS : PARSE_EEND;
}


/* Convert string to uintmax_t and handle errors */
ParseErr stringToUIntMax(uintmax_t *x, char *nptr, uintmax_t min, uintmax_t max, char **endptr, int base)
{
    char sign;

    *endptr = nptr;

    if ((base < 2 && base != 0) || base > 36)
        return PARSE_EBASE;

    /* Get pointer to start of number */
    while (isspace(**endptr))
        ++(*endptr);

    sign = **endptr;

    nptr = *endptr;
    errno = 0;
    *x = strtoumax(nptr, endptr, base);
    
    /* Conversion check */
    if (*endptr == nptr || errno == EINVAL)
        return PARSE_EERR;
    
    /* Range checks */
    if (errno == ERANGE)
        return PARSE_ERANGE;
    else if (*x < min)
        return PARSE_EMIN;
    else if (*x > max)
        return PARSE_EMAX;
    else if (sign == '-' && *x != 0)
        return PARSE_EMIN;

    /* If more characters in string */
    return (**endptr == '\0') ? PARSE_SUCCESS : PARSE_EEND;
}


/* Convert string to double and handle errors */
ParseErr stringToDouble(double *x, char *nptr, double min, double max, char **endptr)
{
    errno = 0;
    *x = strtod(nptr, endptr);
    
    /* Conversion check */
    if (*endptr == nptr)
        return PARSE_EERR;
    
    /* Range checks */
    if (errno == ERANGE)
        return PARSE_ERANGE;
    else if (*x < min)
        return PARSE_EMIN;
    else if (*x > max)
        return PARSE_EMAX;
    
    /* If more characters in string */
    return (**endptr == '\0') ? PARSE_SUCCESS : PARSE_EEND;
}


/* 
 * Parse a string as an imaginary or real double
 *
 * Where:
 *   - The format is that of a `double` type - meaning a decimal, additional 
 *     exponent part, and hexadecimal sequence are all valid inputs
 *   - Whitespace will be stripped
 *   - The operator can be '+' or '-'
 *   - It can be preceded by an optional '+' or '-' sign
 *   - An imaginary number must be followed by the imaginary unit
 */
ParseErr stringToComplexPart(complex *z, char *nptr, complex min, complex max, char **endptr, ComplexPt *type)
{
    double x;
    int sign;
    ParseErr parseError;

    *endptr = nptr;

    /* Get pointer to start of number */
    while (isspace(**endptr))
        ++(*endptr);

    /* 
     * Manually parsing the sign enables detection of a complex unit lacking in
     * a coefficient but having a '+'/'-' sign
     */
    sign = parseSign(*endptr, endptr);

    if (!sign)
        sign = 1;

    /*
     * Because the sign has been manually parsed, error on a second sign, which
     * strtod() will not detect
     */
    if (parseSign(*endptr, endptr))
        return PARSE_EFORM;

    parseError = stringToDouble(&x, *endptr, -(DBL_MAX), DBL_MAX, endptr);

    if (parseError == PARSE_EERR)
    {
        if (toupper(**endptr) != toupper(IMAGINARY_UNIT))
            return PARSE_EFORM;

        /* Failed conversion must be an imaginary unit without coefficient */
        x = 1.0;
    }
    else if (parseError != PARSE_SUCCESS && parseError != PARSE_EEND)
    {
        return parseError;
    }

    x *= sign;

    *type = parseImaginaryUnit(*endptr, endptr);

    parseError = (**endptr == '\0') ? PARSE_SUCCESS : PARSE_EEND;
    
    switch(*type)
    {
        case COMPLEX_REAL:
            if (x < creal(min) || x > creal(max))
                return PARSE_ERANGE;

            *z = x + cimag(*z) * I;
            return parseError;
        case COMPLEX_IMAGINARY:
            if (x < cimag(min) || x > cimag(max))
                return PARSE_ERANGE;

            *z = creal(*z) + x * I;
            return parseError;
        default:
            return PARSE_EERR;
    }
}


/* 
 * Parse a complex number string into a complex variable
 * 
 * Input must be of the form:
 *   "a + bi" or
 *   "bi + a"
 * 
 * Where each part, `a` and `bi`, is parsed according to stringToImaginary():
 *   - The operator can be '+' or '-'
 *   - `a` and `bi` can be preceded by an optional '+' or '-' sign (independant
 *     of the expression's operator)
 *   - There cannot be multiple real or imaginary parts (e.g. "a + b + ci" is
 *     invalid)
 *   - Either parts can be omitted - the missing part will be interpreted as 0.0
 */
ParseErr stringToComplex(complex *z, char *nptr, complex min, complex max, char **endptr)
{
    ComplexPt firstType, secondType;
    char *partEndptr;
    int operator;
    complex secondZPart;

    ParseErr parseError;
 
    *endptr = nptr;

    /* Get pointer to start of number */
    while (isspace(**endptr))
        ++(*endptr);

    *z = 0.0 + 0.0 * I;

    /* Get first operand in complex number */
    parseError = stringToComplexPart(z, *endptr, min, max, endptr, &firstType);

    if (parseError == PARSE_SUCCESS)
        return PARSE_SUCCESS;
    else if (parseError != PARSE_EEND)
        return parseError;

    /* 
     * Record the end of the first part. Any future parse errors should set
     * *endptr back to this and return PARSE_EEND, hence telling the user only
     * the first part was parsed
     */
    partEndptr = *endptr;

    /* Get operator between the two parts */
    operator = parseSign(*endptr, endptr);

    if (!operator)
    {
        *endptr = partEndptr;
        return PARSE_EEND;
    }

    /* Get second operand in complex number */
    parseError = stringToComplexPart(&secondZPart, *endptr, min, max, endptr, &secondType);

    if (parseError != PARSE_SUCCESS && parseError != PARSE_EEND)
    {
        *endptr = partEndptr;
        return PARSE_EEND;
    }

    if (firstType == secondType)
    {
        *endptr = partEndptr;
        return PARSE_EEND;
    }

    /* Set correct part of *z, dependent on the first parsed part's type */
    switch (secondType)
    {
        case COMPLEX_REAL:
            *z = operator * creal(secondZPart) + cimag(*z) * I;
            break;
        case COMPLEX_IMAGINARY:
            *z = creal(*z) + operator * cimag(secondZPart) * I;
            break;
        default:
            *endptr = partEndptr;
            return PARSE_EEND;
    }

    return (**endptr == '\0') ? PARSE_SUCCESS : PARSE_EEND;
}


/* 
 * Parse a positive double with optional memory unit suffix (if omitted,
 * magnitude will be that of the magnitude argument) into a size_t value
 */
ParseErr stringToMemory(size_t *bytes, char *nptr, size_t min, size_t max, char **endptr, int magnitude)
{
    double x;
    int unitPrefix;
    ParseErr parseError;

    *endptr = nptr;

    /* Get pointer to start of number */
    while (isspace(**endptr))
        ++(*endptr);

    parseError = stringToDouble(&x, *endptr, 0.0, DBL_MAX, endptr);

    if (parseError == PARSE_SUCCESS)
    {
        unitPrefix = magnitude;
    }
    else if (parseError == PARSE_EEND)
    {
        nptr = *endptr;
        unitPrefix = parseMemoryUnit(nptr, endptr);

        if (unitPrefix < 0)
        {
            *endptr = nptr;
            unitPrefix = magnitude;
        }
    }
    else
    {
        return parseError;
    }

    x *= pow(10.0, unitPrefix);

    if (x < 0.0 || x > SIZE_MAX)
        return PARSE_ERANGE;

    *bytes = (size_t) x;

    if (*bytes < min)
        return PARSE_EMIN;
    else if (*bytes > max)
        return PARSE_EMAX;

    return (**endptr == '\0') ? PARSE_SUCCESS : PARSE_EEND;
}


static int parseMemoryUnit(char *str, char **endptr)
{
    const char BYTE_UNIT = 'B';
    const char BYTE_PREFIXES[] = {'k', 'M', 'G', 'T', 'P', 'E', 'Z', 'Y'};

    unsigned int magnitude = 0;

    *endptr = str;

    /* Get pointer to start of unit */
    while (isspace(**endptr))
        ++(*endptr);

    for (unsigned int i = 0; i < sizeof(BYTE_PREFIXES) / sizeof(char); ++i)
    {
        if (toupper(**endptr) == toupper(BYTE_PREFIXES[i]))
        {
            magnitude = (i + 1) * 3;

            if (magnitude > INT_MAX)
                return -1;

            ++(*endptr);
            break;
        }
    }

    if (toupper(**endptr) != toupper(BYTE_UNIT))
        return -1;
    
    ++(*endptr);

    /* Cast is safe due to previous check */
    return (int) magnitude;
}


/* Parse the sign of a number */
static int parseSign(char *c, char **endptr)
{
    *endptr = c;

    /* Get pointer to sign */
    while (isspace(**endptr))
        ++(*endptr);

    switch (**endptr)
    {
        case '+':
            ++(*endptr);
            return 1;
        case '-':
            ++(*endptr);
            return -1;
        default:
            return 0;
    }
}


/* Parse the imaginary unit, or lack thereof */
static ComplexPt parseImaginaryUnit(char *c, char **endptr)
{
    *endptr = c;
    
    /* Get pointer to start of imaginary unit */
    while (isspace(**endptr))
        ++(*endptr);

    if (toupper(**endptr) != toupper(IMAGINARY_UNIT))
        return COMPLEX_REAL;

    ++(*endptr);

    return COMPLEX_IMAGINARY;
}


/* 
 * Strip src of non-graphical characters then copy a maximum of n characters
 * (including the null terminator) into dest and return the length of dest
 */
size_t strncpyGraph(char *dest, const char *src, size_t n)
{
    size_t j = 0;

    for (size_t i = 0; src[i] != '\0' && j < n - 1; ++i)
    {
        if (isgraph(src[i]))
            dest[j++] = src[i];
    }

    dest[j] = '\0';

    /* Length of dest */
    return j;
}