dnl config.m4 for extension scoutapm

PHP_ARG_ENABLE([scoutapm],
  [whether to enable scoutapm support],
  [AS_HELP_STRING([--enable-scoutapm],
    [Enable scoutapm support])],
  [no])

if test "$PHP_SCOUT_APM" != "no"; then
  PHP_NEW_EXTENSION(scoutapm,
        zend_scoutapm.c,
        $ext_shared,, ,,yes)
fi

AC_CONFIG_COMMANDS_POST([
  ln -s "$PHP_EXECUTABLE" build/php
])
