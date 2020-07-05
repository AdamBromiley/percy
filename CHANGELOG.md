# Changelog

## 2020-07-05
### Added
- Arbitrary-precision number support via the MPFR and MPC libraries
- `mpfr_t` parsing with `stringToMPFR()`
- `mpc_t` parsing with `stringToMPC()` and `stringToMPCPart()`

## 2020-06-30
### Added
- `long double` parsing with `stringToDoubleL()`
- `long double complex` parsing with `stringToComplexL()` and `stringToComplexPartL()`

## 2020-06-29
### Changed
- `*endptr` argument will now point to final character all the time
- Complex number parsing supports an end-pointer argument now

## 2020-06-26
### Added
- Memory-value parsing with `stringToMemory()`
- Demonstration script in [test/percy_demo.c](test/percy_demo.c)
- Usage instructions in [README.md](README.md)
- `CMPLX_MIN` and `CMPLX_MAX` global constants

## 2020-06-23
### Changed
- Complex and imaginary parsers parse into a `complex` type now
- Refactored the public-facing interface