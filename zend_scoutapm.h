//
// Created by james on 09/08/2019.
//

#ifndef ZEND_SCOUTAPM_H
#define ZEND_SCOUTAPM_H

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "php.h"
#include <zend_extensions.h>
#include <zend_compile.h>
#include <zend_exceptions.h>

#define SCOUT_APM_EXT_NAME "scoutapm"
#define SCOUT_APM_EXT_VERSION "0.0"

// Extreme amounts of debugging, set to 1 to enable it and `make clean && make` (tests will fail...)
#define SCOUT_APM_EXT_DEBUGGING 0

typedef struct scoutapm_stack_frame {
    const char *function_name;
    double entered;
    double exited;
    int argc;
    zval *argv[5];
} scoutapm_stack_frame;

ZEND_BEGIN_MODULE_GLOBALS(scoutapm)
    zend_bool handlers_set;
    zend_long observed_stack_frames_count;
    scoutapm_stack_frame *observed_stack_frames;
ZEND_END_MODULE_GLOBALS(scoutapm)

#ifndef ZEND_PARSE_PARAMETERS_NONE
#define ZEND_PARSE_PARAMETERS_NONE() if (zend_parse_parameters_none() != SUCCESS) { return; }
#endif

#ifdef ZTS
#define SCOUTAPM_G(v) ZEND_MODULE_GLOBALS_ACCESSOR(scoutapm, v)
#else
#define SCOUTAPM_G(v) (scoutapm_globals.v)
#endif

#if SCOUT_APM_EXT_DEBUGGING == 1
#define DEBUG(x, ...) php_printf(x, ##__VA_ARGS__)
#else
#define DEBUG(...) /**/
#endif

#ifndef SCOUT_DEFINE_OVERLOADED_FUNCTION
#define SCOUT_DEFINE_OVERLOADED_FUNCTION(function_name) zif_handler original_handler_##function_name
#endif

#ifndef SCOUT_OVERLOADED_FUNCTION
#define SCOUT_OVERLOADED_FUNCTION(function_name) \
  ZEND_NAMED_FUNCTION(scoutapm_##function_name) \
  { \
    double entered = scoutapm_microtime(); \
    int argc; \
    zval *argv = NULL; \
    \
    ZEND_PARSE_PARAMETERS_START(0, -1) \
    Z_PARAM_VARIADIC('+', argv, argc) \
    ZEND_PARSE_PARAMETERS_END(); \
    \
    original_handler_##function_name(INTERNAL_FUNCTION_PARAM_PASSTHRU); \
    \
    record_observed_stack_frame(#function_name, entered, scoutapm_microtime(), argc, argv); \
  }
#endif

#ifndef SCOUT_OVERLOAD_FUNCTION
#define SCOUT_OVERLOAD_FUNCTION(function_name) \
    zend_function *original_function_##function_name; \
    \
    original_function_##function_name = zend_hash_str_find_ptr(EG(function_table), #function_name, sizeof(#function_name)-1); \
    if (original_function_##function_name != NULL) { \
        original_handler_##function_name = original_function_##function_name->internal_function.handler; \
        original_function_##function_name->internal_function.handler = scoutapm_##function_name; \
    }
#endif

#endif //ZEND_SCOUTAPM_H
