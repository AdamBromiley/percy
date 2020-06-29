# Changelog

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