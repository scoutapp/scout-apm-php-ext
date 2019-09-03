# Scout APM PHP Extension

## Building

```bash
$ phpize
$ ./configure --enable-scoutapm
$ make test
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
