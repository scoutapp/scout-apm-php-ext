# Scout APM PHP Extension

[![PHPT Tests](https://github.com/scoutapp/scout-apm-php-ext/actions/workflows/phpt-tests.yaml/badge.svg)](https://github.com/scoutapp/scout-apm-php-ext/actions/workflows/phpt-tests.yaml)

The Scout APM PHP extension allows instrumentation of internal PHP
functions that can't be done in regular PHP. The `scout-apm-php`
package detects if the `scoutapm` extension is loaded and will
automatically send this data if available.

This extension allows instrumentation of:

 * Core functions: `file_get_contents`, `file_put_contents`, `fread`, `fwrite`
 * Curl functions: `curl_exec`
 * PDO methods: `PDO->exec`, `PDO->query`, `PDOStatement->execute`
 * Predis PHP library methods
 * phpredis PHP extension methods
 * Memcached PHP extension methods
 * Elasticsearch PHP library methods

If you would like another function instrumented, please let us know on
[our issues](https://github.com/scoutapp/scout-apm-php-ext/issues).

The following functions are exposed when the extension is enabled:

 * `scoutapm_enable_instrumentation(bool $enabled): void`
   - Enable or disable instrumentation by ScoutAPM at runtime. Instrumentation is disabled by default, so this must
     be called with `$enabled` to `true`.
 * `scoutapm_get_calls(): array`
   - Returns a list of any instrumented function calls since
     `scoutapm_get_calls()` was last called. The list is cleared each time the
     function is called.
 * `scoutapm_list_instrumented_functions(): array`
   - Returns a list of the functions the extension will instrument if called.

## Installing from PECL

The Scout APM extension is available to install using
[PECL](https://pecl.php.net/package/scoutapm).

```bash
$ sudo pecl install scoutapm
```

You may need to add `zend_extension=scoutapm.so` into your `php.ini` to
enable the extension.

## Building

```bash
$ phpize
$ ./configure --enable-scoutapm
$ make test
```

Run tests with installed PHP (avoids skipped tests):

```bash
make && php run-tests.php -d zend_extension=$(pwd)/modules/scoutapm.so --show-diff -q
```

Note: whilst a CMakeLists.txt exists, this project does NOT use CMake.
The CMakeLists.txt exists so this project can be worked on in CLion.
See <https://dev.to/jasny/developing-a-php-extension-in-clion-3oo1>.

## Building with specific PHP build

```bash
$ /path/to/bin/phpize
$ ./configure --with-php-config=/path/to/bin/php-config --enable-scoutapm
$ make test
```

## Building with Docker

@todo: make docker build configurable...

```bash
docker build .
docker run -v $PWD/modules:/v <hash_from_build>
```

## Debugging

Use `gdb` (e.g. in CLion) to debug. Once running, php-src has a GDB
helper:

```
source /path/to/php-src/.gdbinit
printzv <thing>
print_ht <thing>
zbacktrace
print_cvs
```

## Release Procedure

 - Open `package.xml`
 - Copy the current release into a new `changelog.release` element
 - Update the current release section (date/time/version/stability/notes)
 - `pecl package-validate` to check everything looks good
 - Increase/verify `PHP_SCOUTAPM_VERSION` version listed in `zend_scoutapm.h`
 - Commit update to `package.xml`
 - Rebuild from scratch (`full-clean.sh`, then build as above)
 - `make test` to ensure everything passes locally
 - Push the branch (optionally, make a PR to GitHub) to trigger CI to build
 - Once merged, close the milestone to automatically release & generate the TGZ asset
 - Go to [the latest release](https://github.com/scoutapp/scout-apm-php-ext/releases) just created
 - Download the TGZ asset and upload it to `pecl.php.net`
