# Scout APM PHP Extension

## Building

```bash
$ phpize
$ ./configure --enable-scoutapm
$ make test
```

## Building with specific PHP build

```bash
$ /path/to/bin/phpize
$ ./configure --with-php-config=/path/to/bin/php-config --enable-scoutapm
$ make test
```
