#include "../include/parser.h"

#include <complex.h>
#include <float.h>
#include <getopt.h>
#include <inttypes.h>
#include <limits.h>
#include <math.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

#include <mpfr.h>
#include <mpc.h>


int main(int argc, char **argv)
{
    /* Maximum precision of arbitrary-precision (MPFR) numbers */
    const mpfr_prec_t MPFR_PREC = 512;

    /* Get number of significant digits in MPFR result */
    int mpfrDigits = (int) floor(MPFR_PREC / log2(10));

    /* End pointer of parsed sub-string */
    char *endptr;

    unsigned long ulx;
    uintmax_t uintx;
    double dx;
    mpfr_t mpfrx;
    ComplexPt cmplxPt;
    complex imagx;
    complex cmplxx;
    mpc_t mpcx;
    size_t memx;

    bool u, x, d, D, i, c, C, m;

    char *program = argv[0];

    int optionID;
    const char *GETOPT_STRING = ":u:x:d:D:i:c:C:m:";
    const struct option LONG_OPTIONS[] =
    {
        {"ulong", required_argument, NULL, 'u'},
        {"uintmax", required_argument, NULL, 'x'},
        {"double", required_argument, NULL, 'd'},
        {"mpfr", required_argument, NULL, 'D'},
        {"imaginary", optional_argument, NULL, 'i'},
        {"complex", required_argument, NULL, 'c'},
        {"mpc", required_argument, NULL, 'C'},
        {"memory", required_argument, NULL, 'm'},
        {0, 0, 0, 0}
    };

    u = x = d = D = i = c = C = m = false;

    /* Initialise MPFR and MPC variables */
    mpfr_init2(mpfrx, MPFR_PREC);
    mpc_init2(mpcx, MPFR_PREC);

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
            case 'D':
                D = true;
                err = stringToMPFR(&mpfrx, optarg, NULL, NULL, &endptr, 0, MPFR_RNDN);
                break;
            case 'i':
                i = true;
                err = stringToComplexPart(&imagx, optarg, CMPLX_MIN, CMPLX_MAX, &endptr, &cmplxPt);
                break;
            case 'c':
                c = true;
                err = stringToComplex(&cmplxx, optarg, CMPLX_MIN, CMPLX_MAX, &endptr);
                break;
            case 'C':
                C = true;
                err = stringToComplexMPC(&mpcx, optarg, NULL, NULL, &endptr, 0, MPFR_PREC, MPC_RNDNN);
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
                continue;
            case PARSE_EERR:
                fprintf(stderr, "%s: -%c: Unknown parse error\n", program, optionID);
                break;
            case PARSE_ERANGE:
                fprintf(stderr, "%s: -%c: Argument out of range\n", program, optionID);
                break;
            case PARSE_EMIN:
                fprintf(stderr, "%s: -%c: Argument too small\n", program, optionID);
                break;
            case PARSE_EMAX:
                fprintf(stderr, "%s: -%c: Argument too large\n", program, optionID);
                break;
            case PARSE_EEND:
                fprintf(stderr, "%s: -%c: WARNING: Argument not fully parsed\n", program, optionID);
                continue;
            case PARSE_EBASE:
                fprintf(stderr, "%s: -%c: Invalid conversion radix\n", program, optionID);
                break;
            case PARSE_EFORM:
                fprintf(stderr, "%s: -%c: Incorrect argument format\n", program, optionID);
                break;
            default:
                fprintf(stderr, "%s: -%c: Unknown parse error\n", program, optionID);
                break;
        }

        mpfr_clear(mpfrx);
        mpc_clear(mpcx);
        return 1;
    }

    if (u) printf("Unsigned long        = %lu\n", ulx);
    if (x) printf("Unsigned integer max = %" PRIuMAX "\n", uintx);
    if (d) printf("Double               = %g\n", dx);
    if (D) mpfr_printf("MPFR floating-point  = %.*Rg\n", mpfrDigits, mpfrx);

    if (i) 
    {
        if (cmplxPt == COMPLEX_REAL)
            printf("Complex part         = %f\n", creal(imagx));
        else if (cmplxPt == COMPLEX_IMAGINARY)
            printf("Complex part         = %fi\n", cimag(imagx));
    }

    if (c) printf("Complex              = %g + %gi\n", creal(cmplxx), cimag(cmplxx));

    if (C)
    {
        mpfr_printf("MPC complex          = %.*Rg + %.*Rgi\n",
            mpfrDigits, mpc_realref(mpcx), mpfrDigits, mpc_imagref(mpcx));
    }

    if (m) printf("Memory               = %zu bytes\n", memx);

    mpfr_clear(mpfrx);
    mpc_clear(mpcx);

    return 0;
}