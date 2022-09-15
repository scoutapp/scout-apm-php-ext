dnl config.m4 for extension scoutapm

PHP_ARG_ENABLE([scoutapm],
  [whether to enable scoutapm support],
  [AS_HELP_STRING([--enable-scoutapm],
    [Enable scoutapm support])],
  [no])

PHP_ARG_ENABLE([scoutapm-dev],
  [whether to enable scoutapm developer build flags],
  [AS_HELP_STRING([--enable-scoutapm-dev],
    [Enable scoutapm developer flags])],
  ,[no])

if test "$PHP_SCOUTAPM" != "no"; then

  dnl modern version provides libcurl.pc
  AC_PATH_PROG(PKG_CONFIG, pkg-config, no)
  dnl old version only provides curl-config
  AC_PATH_PROG(CURL_CONFIG, curl-config, no)

  AC_MSG_CHECKING(for libcurl headers)
  if test -x "$PKG_CONFIG" && $PKG_CONFIG --exists libcurl; then
    AC_MSG_RESULT(found with pkg-config)
    CURL_INCL=`$PKG_CONFIG libcurl --cflags`
  elif test -x "$CURL_CONFIG"; then
    AC_MSG_RESULT(found with curl-config)
    CURL_INCL=`$CURL_CONFIG --cflags`
  else
    AC_MSG_WARN(neither pkg-config nor curl-config found)
    CURL_INCL=no
  fi

  if test "$CURL_INCL" = "no"; then
    AC_DEFINE(HAVE_SCOUT_CURL,0,[Curl is present on the system])
    AC_MSG_WARN([curl library headers were not found on the system, scoutapm will not instrument curl functions])
  else
    PHP_EVAL_INCLINE($CURL_INCL)
    AC_DEFINE(HAVE_SCOUT_CURL,1,[Curl is present on the system])
  fi

  if test "$PHP_SCOUTAPM_DEV" = "yes"; then
    MAINTAINER_CFLAGS="-std=gnu99"
    STD_CFLAGS="-g -O0 -Wall"
  fi

  PHP_SCOUTAPM_CFLAGS="-DZEND_ENABLE_STATIC_TSRMLS_CACHE=1 $STD_CFLAGS $MAINTAINER_CFLAGS"

  PHP_NEW_EXTENSION(
    scoutapm,
    zend_scoutapm.c \
    scout_observer.c \
    scout_execute_ex.c \
    scout_internal_handlers.c \
    scout_recording.c \
    scout_functions.c \
    scout_utils.c \
    scout_curl_wrapper.c \
    scout_file_wrapper.c \
    scout_pdo_wrapper.c,
    $ext_shared,,$PHP_SCOUTAPM_CFLAGS,,yes)
fi

AC_CONFIG_COMMANDS_POST([
  rm -f build/php
  ln -s "$PHP_EXECUTABLE" build/php
])
