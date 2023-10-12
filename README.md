# Scout APM PHP Extension

[![CI Build](https://github.com/scoutapp/scout-apm-php-ext/actions/workflows/ci-build.yaml/badge.svg)](https://github.com/scoutapp/scout-apm-php-ext/actions/workflows/ci-build.yaml)

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

## Prerequisites for cURL

In order to have instrumentation for cURL functions enabled, you MUST have `libcurl` available when building or
installing from `pecl`.

For example, if you are using the `ppa:ondrej/php` PPA on Ubuntu, you must install `libcurl` before building `scoutapm`
extension, e.g.:

```bash
$ apt-get -y install libcurl4-openssl-dev
$ pecl install scoutapm
```

To confirm if cURL instrumentation is working, check `php -i`:

```
scoutapm

scoutapm support => enabled
scoutapm Version => 1.9.1
scoutapm curl HAVE_CURL => No
scoutapm curl HAVE_SCOUT_CURL => Yes
scoutapm curl enabled => Yes
```

* `scoutapm curl HAVE_CURL` was PHP itself compiled with cURL. This will not always be `Yes`, for example when using
  pre-packaged binaries where `curl` extension is separate.
* `scoutapm curl HAVE_SCOUT_CURL` was the `scoutapm` extension compiled and `libcurl` available? If this is `No`, then
  cURL instrumentation will not be available at all.
* `scoutapm curl enabled` tells you if `scoutapm` has enabled monitoring (i.e. `curl` extension was available at
  runtime, and `libcurl` was available when `scoutapm` was compiled). _If this value is `No` and you think it
  should be `Yes`, check that `libcurl` was available when `scoutapm` was compiled, and that you have the `curl` PHP
  extension enabled._

## Installing from PECL

The Scout APM extension is available to install using
[PECL](https://pecl.php.net/package/scoutapm).

```bash
$ sudo pecl install scoutapm
```

You may need to add `zend_extension=scoutapm.so` into your `php.ini` to
enable the extension.

## Installing with Docker

If you are using Docker, with the [official PHP images](https://hub.docker.com/_/php), you can install the `scoutapm`
extension using PECL still, for example:

```dockerfile
FROM php:8.2-cli

RUN pecl install scoutapm-1.9.1 \
    && docker-php-ext-enable scoutapm
```

For more information on this installation method, see [here](https://github.com/docker-library/docs/tree/master/php#pecl-extensions).

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

## Windows builds

 - Read this guide to put below into context: https://wiki.php.net/internals/windows/stepbystepbuild_sdk_2
 - Read this guide to help get the environment set up: https://gist.github.com/cmb69/47b8c7fb392f5d79b245c74ac496632c
 - PHP Binary tools - use https://github.com/php/php-sdk-binary-tools (**not** the Microsoft one as it is not maintained)
 - Once the VS tools + php-sdk-binary-tools is installed, everything is done in this shell:
   - Start > `Developer Command Prompt for VS 2019`
   - Then `cd C:\php-sdk`
   - Then `phpsdk-vs16-x64.bat` - you should now have a prompt `$ `
 - Install PHP from https://windows.php.net/download/
   - Download, e.g. ZTS build, https://windows.php.net/downloads/releases/php-8.1.7-Win32-vs16-x64.zip
   - Extract into `C:\php`
 - Prepare to compile the ext
   - Download "Development package" from https://windows.php.net/download/ - make sure TS/NTS depending on above compilation
     - e.g. https://windows.php.net/downloads/releases/php-devel-pack-8.1.7-nts-Win32-vs16-x64.zip
   - Extract to `C:\php-sdk\php-8.1.7-devel-vs16-x64`
   - Add `C:\php-sdk\php-8.1.7-devel-vs16-x64` to your PATH (Start > `env` > Environment variables > "Path" > New)
   - restart the shell
 - Compile the ext - go to the ext directory (mine was a VM, mounted in `Z:\`)
   - Run `winbuild.bat`
   - Or alternatively:
     - `phpize`
     - `configure --enable-scoutapm --enable-debug --with-php-build="C:\php-sdk\phpdev\vs16\x64\deps" --with-prefix="C:\php\"`
     - `nmake`
     - Edit `Makefile` - find `CC="$(PHP_CL)"` and replace with `CC="cl.exe"` - for some reason that variable substitution didn't work
     - Also replace `-d extension=` with `-d zend_extension=`
     - Run `nmake run ARGS="-m"` and check scoutapm exists in both PHP Modules and Zend modules

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
