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
    AX_CHECK_COMPILE_FLAG(-Wbool-conversion,                _MAINTAINER_CFLAGS="$_MAINTAINER_CFLAGS -Wbool-conversion")
    AX_CHECK_COMPILE_FLAG(-Wdeclaration-after-statement,    _MAINTAINER_CFLAGS="$_MAINTAINER_CFLAGS -Wdeclaration-after-statement")
    AX_CHECK_COMPILE_FLAG(-Wdiscarded-qualifiers,           _MAINTAINER_CFLAGS="$_MAINTAINER_CFLAGS -Wdiscarded-qualifiers")
    AX_CHECK_COMPILE_FLAG(-Wduplicate-enum,                 _MAINTAINER_CFLAGS="$_MAINTAINER_CFLAGS -Wduplicate-enum")
    AX_CHECK_COMPILE_FLAG(-Wempty-body,                     _MAINTAINER_CFLAGS="$_MAINTAINER_CFLAGS -Wempty-body")
    AX_CHECK_COMPILE_FLAG(-Wenum-compare,                   _MAINTAINER_CFLAGS="$_MAINTAINER_CFLAGS -Wenum-compare")
    AX_CHECK_COMPILE_FLAG(-Werror,                          _MAINTAINER_CFLAGS="$_MAINTAINER_CFLAGS -Werror")
    AX_CHECK_COMPILE_FLAG(-Wextra,                          _MAINTAINER_CFLAGS="$_MAINTAINER_CFLAGS -Wextra")
    AX_CHECK_COMPILE_FLAG(-Wformat-nonliteral,              _MAINTAINER_CFLAGS="$_MAINTAINER_CFLAGS -Wformat-nonliteral")
    AX_CHECK_COMPILE_FLAG(-Wformat-security,                _MAINTAINER_CFLAGS="$_MAINTAINER_CFLAGS -Wformat-security")
    AX_CHECK_COMPILE_FLAG(-Wheader-guard,                   _MAINTAINER_CFLAGS="$_MAINTAINER_CFLAGS -Wheader-guard")
    AX_CHECK_COMPILE_FLAG(-Wincompatible-pointer-types-discards-qualifiers, _MAINTAINER_CFLAGS="$_MAINTAINER_CFLAGS -Wincompatible-pointer-types-discards-qualifiers")
    AX_CHECK_COMPILE_FLAG(-Wimplicit-fallthrough,           _MAINTAINER_CFLAGS="$_MAINTAINER_CFLAGS -Wimplicit-fallthrough")
    AX_CHECK_COMPILE_FLAG(-Winit-self,                      _MAINTAINER_CFLAGS="$_MAINTAINER_CFLAGS -Winit-self")
    AX_CHECK_COMPILE_FLAG(-Wlogical-not-parentheses,        _MAINTAINER_CFLAGS="$_MAINTAINER_CFLAGS -Wlogical-not-parentheses")
    AX_CHECK_COMPILE_FLAG(-Wlogical-op,                     _MAINTAINER_CFLAGS="$_MAINTAINER_CFLAGS -Wlogical-op")
    AX_CHECK_COMPILE_FLAG(-Wlogical-op-parentheses,         _MAINTAINER_CFLAGS="$_MAINTAINER_CFLAGS -Wlogical-op-parentheses")
    AX_CHECK_COMPILE_FLAG(-Wloop-analysis,                  _MAINTAINER_CFLAGS="$_MAINTAINER_CFLAGS -Wloop-analysis")
    AX_CHECK_COMPILE_FLAG(-Wmaybe-uninitialized,            _MAINTAINER_CFLAGS="$_MAINTAINER_CFLAGS -Wmaybe-uninitialized")
    AX_CHECK_COMPILE_FLAG(-Wmissing-format-attribute,       _MAINTAINER_CFLAGS="$_MAINTAINER_CFLAGS -Wmissing-format-attribute")
    AX_CHECK_COMPILE_FLAG(-Wno-missing-field-initializers,  _MAINTAINER_CFLAGS="$_MAINTAINER_CFLAGS -Wno-missing-field-initializers")
    AX_CHECK_COMPILE_FLAG(-Wno-sign-compare,                _MAINTAINER_CFLAGS="$_MAINTAINER_CFLAGS -Wno-sign-compare")
    AX_CHECK_COMPILE_FLAG(-Wno-unused-but-set-variable,     _MAINTAINER_CFLAGS="$_MAINTAINER_CFLAGS -Wno-unused-but-set-variable")
    AX_CHECK_COMPILE_FLAG(-Wno-unused-parameter,            _MAINTAINER_CFLAGS="$_MAINTAINER_CFLAGS -Wno-unused-parameter")
    AX_CHECK_COMPILE_FLAG(-Wno-variadic-macros,             _MAINTAINER_CFLAGS="$_MAINTAINER_CFLAGS -Wno-variadic-macros")
    AX_CHECK_COMPILE_FLAG(-Wparentheses,                    _MAINTAINER_CFLAGS="$_MAINTAINER_CFLAGS -Wparentheses")
    AX_CHECK_COMPILE_FLAG(-Wpointer-bool-conversion,        _MAINTAINER_CFLAGS="$_MAINTAINER_CFLAGS -Wpointer-bool-conversion")
    AX_CHECK_COMPILE_FLAG(-Wsizeof-array-argument,          _MAINTAINER_CFLAGS="$_MAINTAINER_CFLAGS -Wsizeof-array-argument")
    AX_CHECK_COMPILE_FLAG(-Wstring-conversion,              _MAINTAINER_CFLAGS="$_MAINTAINER_CFLAGS -Wstring-conversion")
    AX_CHECK_COMPILE_FLAG(-Wwrite-strings,                  _MAINTAINER_CFLAGS="$_MAINTAINER_CFLAGS -Wwrite-strings")
    AX_CHECK_COMPILE_FLAG(-fdiagnostics-show-option,        _MAINTAINER_CFLAGS="$_MAINTAINER_CFLAGS -fdiagnostics-show-option")
    AX_CHECK_COMPILE_FLAG(-fno-exceptions,                  _MAINTAINER_CFLAGS="$_MAINTAINER_CFLAGS -fno-exceptions")
    AX_CHECK_COMPILE_FLAG(-fno-omit-frame-pointer,          _MAINTAINER_CFLAGS="$_MAINTAINER_CFLAGS -fno-omit-frame-pointer")
    AX_CHECK_COMPILE_FLAG(-fno-optimize-sibling-calls,      _MAINTAINER_CFLAGS="$_MAINTAINER_CFLAGS -fno-optimize-sibling-calls")
    AX_CHECK_COMPILE_FLAG(-fsanitize-address,               _MAINTAINER_CFLAGS="$_MAINTAINER_CFLAGS -fsanitize-address")
    AX_CHECK_COMPILE_FLAG(-fstack-protector,                _MAINTAINER_CFLAGS="$_MAINTAINER_CFLAGS -fstack-protector")
    MAINTAINER_CFLAGS="$_MAINTAINER_CFLAGS"
    STD_CFLAGS="-g -O0 -Wall"
  fi

  PHP_SCOUTAPM_CFLAGS="-DZEND_ENABLE_STATIC_TSRMLS_CACHE=1 $STD_CFLAGS $MAINTAINER_CFLAGS"

  PHP_NEW_EXTENSION(scoutapm, zend_scoutapm.c scout_observer.c scout_functions.c scout_recording.c scout_utils.c scout_curl_wrapper.c scout_file_wrapper.c scout_pdo_wrapper.c,
        $ext_shared,,$PHP_SCOUTAPM_CFLAGS,,yes)
fi

AC_CONFIG_COMMANDS_POST([
  rm -f build/php
  ln -s "$PHP_EXECUTABLE" build/php
])
