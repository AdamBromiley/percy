#include "parser.h"

#include <complex.h>
#include <ctype.h>
#include <errno.h>
#include <float.h>
#include <inttypes.h>
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
static size_t stripWhitespace(char *dest, const char *src, size_t n);


/* Convert string to unsigned long and handle errors */
ParseErr stringToULong(unsigned long *x, const char *nptr, unsigned long min, unsigned long max, char **endptr, int base)
{
    char sign;

    if ((base < 2 && base != 0) || base > 36)
        return PARSE_EBASE;

    /* Get pointer to start of number */
    while (isspace(*nptr))
        nptr++;

    sign = *nptr;

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

    /* Successful but more characters in string */
    if (**endptr != '\0')
        return PARSE_EEND;

    return PARSE_SUCCESS;
}


/* Convert string to uintmax_t and handle errors */
ParseErr stringToUIntMax(uintmax_t *x, const char *nptr, uintmax_t min, uintmax_t max, char **endptr, int base)
{
    char sign;

    if ((base < 2 && base != 0) || base > 36)
        return PARSE_EBASE;

    /* Get pointer to start of number */
    while (isspace(*nptr))
        nptr++;

    sign = *nptr;

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

    /* Successful but more characters in string */
    if (**endptr != '\0')
        return PARSE_EEND;

    return PARSE_SUCCESS;
}


/* Convert string to double and handle errors */
ParseErr stringToDouble(double *x, const char *nptr, double min, double max, char **endptr)
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
    
    /* Successful but more characters in string */
    if (**endptr != '\0')
        return PARSE_EEND;

    return PARSE_SUCCESS;
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
    ParseErr parseError;

    /* 
     * Manually parsing the sign enables detection of a complex unit lacking in
     * a coefficient but having a '+'/'-' sign
     */
    int sign = parseSign(nptr, endptr);

    if (!sign)
        sign = 1;

    /*
     * Because the sign has been manually parsed, error on a second sign, which
     * strtod() will not detect
     */
    nptr = *endptr;

    if (parseSign(nptr, endptr))
        return PARSE_EFORM;

    nptr = *endptr;
    parseError = stringToDouble(&x, nptr, -(DBL_MAX), DBL_MAX, endptr);

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
    
    nptr = *endptr;
    *type = parseImaginaryUnit(nptr, endptr);
    
    switch(*type)
    {
        case COMPLEX_REAL:
            if (x < creal(min) || x > creal(max))
                return PARSE_ERANGE;
            *z = x + cimag(*z) * I;
            return PARSE_SUCCESS;
        case COMPLEX_IMAGINARY:
            if (x < cimag(min) || x > cimag(max))
                return PARSE_ERANGE;
            *z = creal(*z) + x * I;
            return PARSE_SUCCESS;
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
    int operator;
    ComplexPt type;
    bool reFlag = false, imFlag = false;

    ParseErr parseError;

    size_t bufferSize = strlen(nptr) + 1;
    char *buffer = malloc(bufferSize);
    
    if (!buffer)
        return PARSE_EERR;

    stripWhitespace(buffer, nptr, bufferSize);

    nptr = buffer;
    *endptr = nptr;

    *z = 0.0 + 0.0 * I;

    /* Get first operand in complex number */
    parseError = stringToComplexPart(z, nptr, min, max, endptr, &type);

    if (parseError != PARSE_SUCCESS)
    {
        free(buffer);
        return parseError;
    }

    switch (type)
    {
        case COMPLEX_REAL:
            reFlag = 1;
            break;
        case COMPLEX_IMAGINARY:
            imFlag = 1;
            break;
        default:
            free(buffer);
            return PARSE_EERR;
    }

    if (**endptr == '\0')
    {
        free(buffer);
        return PARSE_SUCCESS;
    }

    /* Get operator between the two parts */
    nptr = *endptr;
    operator = parseSign(nptr, endptr);

    if (!operator)
    {
        free(buffer);
        return PARSE_EFORM;
    }

    /* Get second operand in complex number */
    nptr = *endptr;
    parseError = stringToComplexPart(z, nptr, min, max, endptr, &type);

    if (parseError != PARSE_SUCCESS)
    {
        free(buffer);
        return parseError;
    }

    switch (type)
    {
        case COMPLEX_REAL:
            reFlag = 1;
            *z = operator * creal(*z) + cimag(*z) * I;
            break;
        case COMPLEX_IMAGINARY:
            imFlag = 1;
            *z = creal(*z) + operator * cimag(*z) * I;
            break;
        default:
            free(buffer);
            return PARSE_EERR;
    }

    if (**endptr != '\0')
    {
        free(buffer);
        return PARSE_EEND;
    }

    free(buffer);

    /* If both flags have not been set */
    if (!reFlag || !imFlag)
        return PARSE_EERR;

    return PARSE_SUCCESS;
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

    size_t bufferSize = strlen(nptr) + 1;
    char *buffer = malloc(bufferSize);

    if (!buffer)
        return PARSE_EERR;

    stripWhitespace(buffer, nptr, bufferSize);

    nptr = buffer;
    *endptr = nptr;

    parseError = stringToDouble(&x, nptr, 0.0, DBL_MAX, endptr);

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
            free(buffer);
            return PARSE_EFORM;
        }
    }
    else
    {
        free(buffer);
        return parseError;
    }

    x *= pow(10.0, unitPrefix);

    if (x < 0.0 || x > SIZE_MAX)
    {
        free(buffer);
        return PARSE_ERANGE;
    }

    *bytes = (size_t) x;

    if (*bytes < min)
    {
        free(buffer);
        return PARSE_EMIN;
    }
    else if (*bytes > max)
    {
        free(buffer);
        return PARSE_EMAX;
    }

    if (**endptr != '\0')
    {
        free(buffer);
        return PARSE_EEND;
    }
    
    free(buffer);

    return PARSE_SUCCESS;
}


static int parseMemoryUnit(char *str, char **endptr)
{
    const char BYTE_UNIT = 'B';
    const char BYTE_PREFIXES[] = {'k', 'M', 'G', 'T', 'P', 'E', 'Z', 'Y'};
    
    int magnitude = 0;

    *endptr = str;

    for (unsigned int i = 0; i < sizeof(BYTE_PREFIXES) / sizeof(char); ++i)
    {
        if (toupper(*str) == toupper(BYTE_PREFIXES[i]))
        {
            magnitude = ((signed) i + 1) * 3;
            ++(*endptr);
            break;
        }
    }

    if (toupper(**endptr) != toupper(BYTE_UNIT))
        return -1;
    
    ++(*endptr);

    return magnitude;
}


/* Parse the sign of a number */
static int parseSign(char *c, char **endptr)
{
    *endptr = c;

    switch (*c)
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

    if (toupper(*c) != toupper(IMAGINARY_UNIT))
        return COMPLEX_REAL;

    ++(*endptr);

    return COMPLEX_IMAGINARY;
}


/* 
 * Strip src of white-space then copy a maximum of n characters (including the
 * null terminator) into dest - return the length of dest
 */
static size_t stripWhitespace(char *dest, const char *src, size_t n)
{
    size_t j = 0;

    for (size_t i = 0; src[i] != '\0' && j < n - 1; ++i)
    {
        if (!isspace(src[i]))
            dest[j++] = src[i];
    }

    dest[j] = '\0';

    /* Length of dest */
    return j;
}