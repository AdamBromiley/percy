#include "../include/parser.h"

#include <complex.h>
#include <float.h>
#include <getopt.h>
#include <inttypes.h>
#include <limits.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>


int main(int argc, char **argv)
{
    char *endptr;

    unsigned long ulx;
    uintmax_t uintx;
    double dx;
    ComplexPt cmplxPt;
    complex imagx;
    complex cmplxx;
    size_t memx;

    bool u, x, d, i, c, m;
    u = x = d = i = c = m = false;

    char *program = argv[0];

    int optionID;
    const char *GETOPT_STRING = ":u:x:d:i:c:m:";
    const struct option LONG_OPTIONS[] =
    {
        {"ulong", required_argument, NULL, 'u'},
        {"uintmax", required_argument, NULL, 'x'},
        {"double", required_argument, NULL, 'd'},
        {"imaginary", optional_argument, NULL, 'i'},
        {"complex", required_argument, NULL, 'c'},
        {"memory", required_argument, NULL, 'm'},
        {0, 0, 0, 0}
    };

    opterr = 0;
    while ((optionID = getopt_long(argc, argv, GETOPT_STRING, LONG_OPTIONS, NULL)) != -1)
    {
        ParseErr err = PARSE_SUCCESS;

        switch (optionID)
        {
            case 'u':
                u = true;
                err = stringToULong(&ulx, optarg, 0, ULONG_MAX, &endptr, BASE_DEC);
                break;
            case 'x':
                x = true;
                err = stringToUIntMax(&uintx, optarg, 0, UINTMAX_MAX, &endptr, BASE_DEC);
                break;
            case 'd':
                d = true;
                err = stringToDouble(&dx, optarg, -(DBL_MAX), DBL_MAX, &endptr);
                break;
            case 'i':
                i = true;
                err = stringToComplexPart(&imagx, optarg, CMPLX_MIN, CMPLX_MAX, &endptr, &cmplxPt);
                break;
            case 'c':
                c = true;
                err = stringToComplex(&cmplxx, optarg, CMPLX_MIN, CMPLX_MAX, &endptr);
                break;
            case 'm':
                m = true;
                err = stringToMemory(&memx, optarg, 0, SIZE_MAX, &endptr, MEM_MB);
                break;
            default:
                continue;
        }

        switch (err)
        {
            case PARSE_SUCCESS:
                break;
            case PARSE_EERR:
                fprintf(stderr, "%s: -%c: Unknown parse error\n", program, optionID);
                return 1;
            case PARSE_ERANGE:
                fprintf(stderr, "%s: -%c: Argument out of range\n", program, optionID);
                return 1;
            case PARSE_EMIN:
                fprintf(stderr, "%s: -%c: Argument too small\n", program, optionID);
                return 1;
            case PARSE_EMAX:
                fprintf(stderr, "%s: -%c: Argument too large\n", program, optionID);
                return 1;
            case PARSE_EEND:
                fprintf(stderr, "%s: -%c: WARNING: Argument not fully parsed\n", program, optionID);
                break;
            case PARSE_EBASE:
                fprintf(stderr, "%s: -%c: Invalid conversion radix\n", program, optionID);
                return 1;
            case PARSE_EFORM:
                fprintf(stderr, "%s: -%c: Incorrect argument format\n", program, optionID);
                return 1;
            default:
                fprintf(stderr, "%s: -%c: Unknown parse error\n", program, optionID);
                return 1;
        }
    }

    if (u) printf("Unsigned long        = %lu\n", ulx);
    if (x) printf("Unsigned integer max = %" PRIuMAX "\n", uintx);
    if (d) printf("Double               = %g\n", dx);

    if (i) 
    {
        if (cmplxPt == COMPLEX_REAL)
            printf("Complex part         = %f\n", creal(imagx));
        else if (cmplxPt == COMPLEX_IMAGINARY)
            printf("Complex part         = %fi\n", cimag(imagx));
    }

    if (c) printf("Complex              = %g + %gi\n", creal(cmplxx), cimag(cmplxx));
    if (m) printf("Memory               = %zu bytes\n", memx);

    return 0;
}