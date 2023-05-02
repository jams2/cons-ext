# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

## [0.4.0] - 2023-05-02

### Added

- `Cons_lift` method
- `assoc` function
- `assp` function

### Changed

- Don't create intermediate tuple when calling `cons.from_xs` on a generator

## [0.3.0] - 2023-04-20

### Added

- `Cons_hash` method

## [0.2.1] - 2023-04-09

### Fixed

- Memory leak in `Cons_from_fast`

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
