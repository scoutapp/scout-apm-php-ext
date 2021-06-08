# Changelog

All notable changes to this project will be documented in this file, in reverse chronological order by release.

## 1.3.0 - TBC

### Added

- [#77](https://github.com/scoutapp/scout-apm-php-ext/pull/77) Userland function recording for PHP 7 with zend_execute_ex
- [#79](https://github.com/scoutapp/scout-apm-php-ext/pull/79) Userland function recording for PHP 8 with improved Zend Observer API
- [#80](https://github.com/scoutapp/scout-apm-php-ext/pull/80) Added Predis library function instrumentation

### Changed

- [#78](https://github.com/scoutapp/scout-apm-php-ext/pull/78) Change CI from Circle to GitHub Actions

### Deprecated

- Nothing.

### Removed

- Nothing.

### Fixed

- Nothing.

## 1.2.2 - 2021-03-19

### Added

- Nothing.

### Changed

- Nothing.

### Deprecated

- Nothing.

### Removed

- Nothing.

### Fixed

- [#72](https://github.com/scoutapp/scout-apm-php-ext/pull/72) Do not try to record arguments if PDO::prepare returns a non-object

## 1.2.1 - 2021-02-04

### Added

- Nothing.

### Changed

- Nothing.

### Deprecated

- Nothing.

### Removed

- Nothing.

### Fixed

- [#69](https://github.com/scoutapp/scout-apm-php-ext/pull/69) Fixing builds on ZTS mode (thanks @remicollet)

## 1.2.0 - 2021-02-04

### Added

- [#66](https://github.com/scoutapp/scout-apm-php-ext/pull/66) Added support for PHP 8.0

### Changed

- Nothing.

### Deprecated

- Nothing.

### Removed

- Nothing.

### Fixed

- Nothing.

## 1.1.1 - 2020-02-19

### Added

- Nothing.

### Changed

- Nothing.

### Deprecated

- Nothing.

### Removed

- Nothing.

### Fixed

- [#62](https://github.com/scoutapp/scout-apm-php-ext/pull/62) Fixed typo in config.m4 for libcurl detection (thanks @remicollet)

## 1.1.0 - 2020-02-19

### Added

- Nothing.

### Changed

- [#60](https://github.com/scoutapp/scout-apm-php-ext/pull/60) Added support for PHP 7.4
- [#58](https://github.com/scoutapp/scout-apm-php-ext/pull/58) Improved checking for curl at compile time - thanks @remicollet

### Deprecated

- Nothing.

### Removed

- Nothing.

### Fixed

- Nothing.

## 1.0.2 - 2019-11-26

### Added

- Nothing.

### Changed

- Nothing.

### Deprecated

- Nothing.

### Removed

- Nothing.

### Fixed

- [#54](https://github.com/scoutapp/scout-apm-php-ext/pull/54) Check if curl header is available and compile support with/without curl accordingly
- [#56](https://github.com/scoutapp/scout-apm-php-ext/pull/56) Fixed bug when overriding methods in extended classes (e.g. doctrine/dbal)

## 1.0.1 - 2019-11-06

### Added

- Nothing.

### Changed

- Nothing.

### Deprecated

- Nothing.

### Removed

- [#51](https://github.com/scoutapp/scout-apm-php-ext/pull/51) Removed notice emitted calling some functions

### Fixed

- [#48](https://github.com/scoutapp/scout-apm-php-ext/pull/48) Fix segfault when trying to access args out of bounds
- [#50](https://github.com/scoutapp/scout-apm-php-ext/pull/50) Fix exception raised when trying to fopen a file that does not exist

## 1.0.0 - 2019-11-04

### Added

- [#38](https://github.com/scoutapp/scout-apm-php-ext/pull/38) More documentation into README.md

### Changed

- [#42](https://github.com/scoutapp/scout-apm-php-ext/pull/42) [#43](https://github.com/scoutapp/scout-apm-php-ext/pull/43) Improved argument handling for functions like `curl_exec`, `fwrite`, `fread`, `PDOStatement->execute`
- [#40](https://github.com/scoutapp/scout-apm-php-ext/pull/40) Better text matrix introduced, including PHP 7.4 tests

### Deprecated

- Nothing.

### Removed

- Nothing.

### Fixed

- [#44](https://github.com/scoutapp/scout-apm-php-ext/pull/44) Bug fixes for [#41](https://github.com/scoutapp/scout-apm-php-ext/issues/41) and [#29](https://github.com/scoutapp/scout-apm-php-ext/issues/29) to help prevent bad configuration of overwritten functions

## 0.0.4 - 2019-09-27

### Added

- Nothing.

### Changed

- Nothing.

### Deprecated

- Nothing.

### Removed

- Nothing.

### Fixed

- Define i/j etc to follow c89 rules (thanks @remicollet)
- Fixed test failing because differing behaviour of sqlite in some versions

## 0.0.3 - 2019-09-17

### Added

- Nothing.

### Changed

- Nothing.

### Deprecated

- Nothing.

### Removed

- Nothing.

### Fixed

- Fixed version number and naming convention so PECL uploader picks up on mismatches (last release was wrong)

## 0.0.2 - 2019-09-17

### Added

- Added extra compiler flags in development mode with --enable-scoutapm-dev
- Added missing file external.inc in tests

### Changed

- Nothing.

### Deprecated

- Nothing.

### Removed

- Nothing.

### Fixed

- Fixed compilation errors surfaced by --enable-scoutapm-dev option

## 0.0.1 - 2019-09-17

### Added

- Basic monitoring of `file_get_contents`, `file_put_contents`, `fwrite`, `fread`, `curl_exec`, `PDO->exec`, `PDO->query`, `PDOStatement->execute`
- Provides function `scoutapm_get_calls()` to return a list of recorded function calls

### Changed

- Nothing.

### Deprecated

- Nothing.

### Removed

- Nothing.

### Fixed

- Nothing.
