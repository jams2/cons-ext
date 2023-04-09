# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

## [0.3.0] - 2023-xx-xx

### Added

- Freelist implementation

### Fixed

- Memory leak caused by extra refcount in `Cons_from_fast`

## [0.2.0] - 2023-04-17

### Added

- `cons.to_list` method
- `CAR` and `CDR` convenience macros

### Fixed

- Cons_richcompare returning incorrect results for `!=`

## [0.1.3] - 2023-04-06

### Fixed

- Incorrect usage of `Py_RETURN_RICHCOMPARE`

## [0.1.0] - 2023-04-06

### Added

- Initial implementation
