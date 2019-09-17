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
 - Commit update to `package.xml`
 - Rebuild from scratch (`full-clean.sh`, then build as above)
 - `make test` to ensure everything passes locally
 - Push the branch (optionally, make a PR to GitHub) to trigger CI to build
 - Once passed, `git tag -s <tagversion>` and enter changelog notes in tag body
 - `git push origin <tagversion>` to push the tag up
 - [Create a release](https://github.com/scoutapp/scout-apm-php-ext/releases/new) from the tag (use same changelog notes)
 - `pecl package` to generate the tgz
 - Upload to `pecl.php.net`
 - Shift the milestones along on GitHub
