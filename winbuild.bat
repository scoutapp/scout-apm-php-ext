rmdir /S /Q x64
call phpize
call configure --enable-scoutapm --enable-debug --with-php-build="C:\php-sdk\phpdev\vs16\x64\deps" --with-prefix="C:\php\"
call sed -i 's/-d extension=/-d zend_extension=/g' Makefile
call nmake CC=cl.exe
call nmake run ARGS="-m"
