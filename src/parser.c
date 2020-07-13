#include "parser.h"

#include <assert.h>
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

#ifdef MP_PREC
#include <mpfr.h>
#include <mpc.h>
#endif


/* Minimum/maximum possible complex values */
const complex CMPLX_MIN = -(DBL_MAX) - DBL_MAX * I;
const complex CMPLX_MAX = DBL_MAX + DBL_MAX * I;

/* Minimum/maximum possible long double complex values */
const long double complex LCMPLX_MIN = -(LDBL_MAX) - LDBL_MAX * I;
const long double complex LCMPLX_MAX = LDBL_MAX + LDBL_MAX * I;


/* Symbol to denote the imaginary unit (case-insensitive) */
static const char IMAGINARY_UNIT = 'i';


static int parseMemoryUnit(char *str, char **endptr);
static int parseSign(char *c, char **endptr);
static ComplexPt parseImaginaryUnit(char *c, char **endptr);

#ifdef MP_PREC
static mpfr_rnd_t getReMPFRRound(mpc_rnd_t rnd);
static mpfr_rnd_t getImMPFRRound(mpc_rnd_t rnd);
#endif


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


/* Convert string to long double and handle errors */
ParseErr stringToDoubleL(long double *x, char *nptr, long double min, long double max, char **endptr)
{
    errno = 0;
    *x = strtold(nptr, endptr);
    
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

    switch(*type)
    {
        case COMPLEX_REAL:
            if (x < creal(min))
                return PARSE_EMIN;
            else if (x > creal(max))
                return PARSE_EMAX;

            *z = x + cimag(*z) * I;
            break;
        case COMPLEX_IMAGINARY:
            if (x < cimag(min))
                return PARSE_EMIN;
            else if (x > cimag(max))
                return PARSE_EMAX;

            *z = creal(*z) + x * I;
            break;
        default:
            return PARSE_EERR;
    }

    return (**endptr == '\0') ? PARSE_SUCCESS : PARSE_EEND;
}


/* 
 * Parse a string as an imaginary or real long double
 *
 * Where:
 *   - The format is that of a `double` type - meaning a decimal, additional 
 *     exponent part, and hexadecimal sequence are all valid inputs
 *   - Whitespace will be stripped
 *   - The operator can be '+' or '-'
 *   - It can be preceded by an optional '+' or '-' sign
 *   - An imaginary number must be followed by the imaginary unit
 */
ParseErr stringToComplexPartL(long double complex *z, char *nptr, long double complex min, long double complex max,
                                char **endptr, ComplexPt *type)
{
    long double x;
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

    parseError = stringToDoubleL(&x, *endptr, -(LDBL_MAX), LDBL_MAX, endptr);

    if (parseError == PARSE_EERR)
    {
        if (toupper(**endptr) != toupper(IMAGINARY_UNIT))
            return PARSE_EFORM;

        /* Failed conversion must be an imaginary unit without coefficient */
        x = 1.0L;
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
            if (x < creall(min) || x > creall(max))
                return PARSE_ERANGE;

            *z = x + cimagl(*z) * I;
            return parseError;
        case COMPLEX_IMAGINARY:
            if (x < cimagl(min) || x > cimagl(max))
                return PARSE_ERANGE;

            *z = creall(*z) + x * I;
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
 * Parse a complex number string into a long double complex variable
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
ParseErr stringToComplexL(long double complex *z, char *nptr, long double complex min, long double complex max,
                             char **endptr)
{
    ComplexPt firstType, secondType;
    char *partEndptr;
    int operator;
    long double complex secondZPart;

    ParseErr parseError;

    *endptr = nptr;

    /* Get pointer to start of number */
    while (isspace(**endptr))
        ++(*endptr);

    *z = 0.0L + 0.0L * I;

    /* Get first operand in complex number */
    parseError = stringToComplexPartL(z, *endptr, min, max, endptr, &firstType);

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
    parseError = stringToComplexPartL(&secondZPart, *endptr, min, max, endptr, &secondType);

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
            *z = operator * creall(secondZPart) + cimagl(*z) * I;
            break;
        case COMPLEX_IMAGINARY:
            *z = creall(*z) + operator * cimagl(secondZPart) * I;
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


#ifdef MP_PREC
/* Convert string to MPFR floating-point and handle errors */
ParseErr stringToMPFR(mpfr_t x, char *nptr, mpfr_t min, mpfr_t max, char **endptr, int base, mpfr_rnd_t rnd)
{
    mpfr_flags_t mpfrErr;

    if ((base < 2 && base != 0) || base > 62)
        return PARSE_EBASE;

    mpfr_clear_flags();

    mpfr_strtofr(x, nptr, endptr, base, rnd);

    /* Inexactness is not considered an error */
    mpfr_clear_inexflag();
    mpfrErr = mpfr_flags_save();
    
    if (mpfrErr || *endptr == nptr)
    {
        if (mpfrErr & MPFR_FLAGS_UNDERFLOW
            || mpfrErr & MPFR_FLAGS_OVERFLOW
            || mpfrErr & MPFR_FLAGS_ERANGE)
        {
            return PARSE_ERANGE;
        }

        return PARSE_EERR;
    }

    /* If user supplied minimum and/or maximum */
    if (min && mpfr_cmp(x, min) < 0)
        return PARSE_EMIN;
    
    if (max && mpfr_cmp(x, max) > 0)
        return PARSE_EMAX;

    /* If more characters in string */
    return (**endptr == '\0') ? PARSE_SUCCESS : PARSE_EEND;
}


/* 
 * Parse a string as an imaginary or real MPFR floating-point
 *
 * Where:
 *   - The format is that of an `mpc_t` type - meaning a decimal, additional 
 *     exponent part, and hexadecimal sequence are all valid inputs
 *   - Whitespace will be stripped
 *   - The operator can be '+' or '-'
 *   - It can be preceded by an optional '+' or '-' sign
 *   - An imaginary number must be followed by the imaginary unit
 */
ParseErr stringToComplexPartMPC(mpc_t z, char *nptr, mpc_t min, mpc_t max, char **endptr,
                                   int base, mpfr_prec_t prec, mpc_rnd_t rnd, ComplexPt *type)
{
    mpfr_t x;
    int sign;
    ParseErr parseError;

    char *tmpptr;
    mpfr_rnd_t mpfrRnd;

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
     * gmp_sscanf() will not detect
     */
    if (parseSign(*endptr, endptr))
        return PARSE_EFORM;

    mpfr_init2(x, prec);

    /* Do a dummy read of the number to apply correct rounding mode */
    tmpptr = *endptr;
    stringToMPFR(x, *endptr, NULL, NULL, endptr, base, MPFR_RNDN);

    if (parseImaginaryUnit(*endptr, endptr) == COMPLEX_IMAGINARY)
        mpfrRnd = getImMPFRRound(rnd);
    else
        mpfrRnd = getReMPFRRound(rnd);
    
    if (mpfrRnd == MPFR_RNDA)
    {
        mpfr_clear(x);
        return PARSE_EERR;
    }

    *endptr = tmpptr;
    parseError = stringToMPFR(x, *endptr, NULL, NULL, endptr, base, mpfrRnd);

    if (parseError == PARSE_EERR || parseError == PARSE_EFORM)
    {
        if (toupper(**endptr) != toupper(IMAGINARY_UNIT))
        {
            mpfr_clear(x);
            return PARSE_EFORM;
        }

        /* Failed conversion must be an imaginary unit without coefficient */
        mpfr_set_d(x, 1.0, mpfrRnd);
    }
    else if (parseError != PARSE_SUCCESS && parseError != PARSE_EEND)
    {
        mpfr_clear(x);
        return parseError;
    }

    if (sign == -1)
        mpfr_neg(x, x, mpfrRnd);

    *type = parseImaginaryUnit(*endptr, endptr);

    switch(*type)
    {
        case COMPLEX_REAL:
            if (min && mpfr_cmp(x, mpc_realref(min)) < 0)
            {
                mpfr_clear(x);
                return PARSE_EMIN;
            }
            else if (max && mpfr_cmp(x, mpc_realref(max)) > 0)
            {
                mpfr_clear(x);
                return PARSE_EMAX;
            }

            mpc_set_fr_fr(z, x, mpc_imagref(z), rnd);

            break;
        case COMPLEX_IMAGINARY:
            if (min && mpfr_cmp(x, mpc_imagref(min)) < 0)
            {
                mpfr_clear(x);
                return PARSE_EMIN;
            }
            else if (max && mpfr_cmp(x, mpc_imagref(max)) > 0)
            {
                mpfr_clear(x);
                return PARSE_EMAX;
            }

            mpc_set_fr_fr(z, mpc_realref(z), x, rnd);

            break;
        default:
            mpfr_clear(x);
            return PARSE_EERR;
    }

    mpfr_clear(x);

    return (**endptr == '\0') ? PARSE_SUCCESS : PARSE_EEND;
}


/* 
 * Parse a complex number string into an MPC complex variable
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
ParseErr stringToComplexMPC(mpc_t z, char *nptr, mpc_t min, mpc_t max, char **endptr,
                               int base, mpfr_prec_t prec, mpc_rnd_t rnd)
{
    ComplexPt firstType, secondType;
    char *partEndptr;
    int operator;
    mpc_t secondZPart;

    ParseErr parseError;
 
    *endptr = nptr;

    /* Get pointer to start of number */
    while (isspace(**endptr))
        ++(*endptr);

    mpc_set_d_d(z, 0.0, 0.0, rnd);

    /* Get first operand in complex number */
    parseError = stringToComplexPartMPC(z, *endptr, min, max, endptr, base, prec, rnd, &firstType);

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

    mpc_init2(secondZPart, prec);

    /* Get second operand in complex number */
    parseError = stringToComplexPartMPC(secondZPart, *endptr, min, max, endptr, base, prec, rnd, &secondType);

    if (parseError != PARSE_SUCCESS && parseError != PARSE_EEND)
    {
        *endptr = partEndptr;
        mpc_clear(secondZPart);
        return PARSE_EEND;
    }

    if (firstType == secondType)
    {
        *endptr = partEndptr;
        mpc_clear(secondZPart);
        return PARSE_EEND;
    }

    if (operator == -1)
        mpc_neg(secondZPart, secondZPart, rnd);

    /* Set correct part of z, dependent on the first parsed part's type */
    switch (secondType)
    {
        case COMPLEX_REAL:
            mpc_set_fr_fr(z, mpc_realref(secondZPart), mpc_imagref(z), rnd);
            break;
        case COMPLEX_IMAGINARY:
            mpc_set_fr_fr(z, mpc_realref(z), mpc_imagref(secondZPart), rnd);
            break;
        default:
            *endptr = partEndptr;
            mpc_clear(secondZPart);
            return PARSE_EEND;
    }

    mpc_clear(secondZPart);

    return (**endptr == '\0') ? PARSE_SUCCESS : PARSE_EEND;
}
#endif


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


#ifdef MP_PREC
/* Get real rounding mode from MPC mode */
static mpfr_rnd_t getReMPFRRound(mpc_rnd_t rnd)
{
    if (rnd == MPC_RNDNN || rnd == MPC_RNDNZ || rnd == MPC_RNDNU || rnd == MPC_RNDND)
        return MPFR_RNDN;
    else if (rnd == MPC_RNDZN || rnd == MPC_RNDZZ || rnd == MPC_RNDZU || rnd == MPC_RNDZD)
        return MPFR_RNDZ;
    else if (rnd == MPC_RNDUN || rnd == MPC_RNDUZ || rnd == MPC_RNDUU || rnd == MPC_RNDUD)
        return MPFR_RNDU;
    else if (rnd == MPC_RNDDN || rnd == MPC_RNDDZ || rnd == MPC_RNDDU || rnd == MPC_RNDDD)
        return MPFR_RNDD;

    /* Return unused (in MPC) MPFR rounding mode on error */
    return MPFR_RNDA;
}


/* Get imaginary rounding mode from MPC mode */
static mpfr_rnd_t getImMPFRRound(mpc_rnd_t rnd)
{
    if (rnd == MPC_RNDNN || rnd == MPC_RNDZN || rnd == MPC_RNDUN || rnd == MPC_RNDDN)
        return MPFR_RNDN;
    else if (rnd == MPC_RNDNZ || rnd == MPC_RNDZZ || rnd == MPC_RNDUZ || rnd == MPC_RNDDZ)
        return MPFR_RNDZ;
    else if (rnd == MPC_RNDNU || rnd == MPC_RNDZU || rnd == MPC_RNDUU || rnd == MPC_RNDDU)
        return MPFR_RNDU;
    else if (rnd == MPC_RNDND || rnd == MPC_RNDZD || rnd == MPC_RNDUD || rnd == MPC_RNDDD)
        return MPFR_RNDD;

    /* Return unused (in MPC) MPFR rounding mode on error */
    return MPFR_RNDA;
}
#endif