/*
 * Scout APM extension for PHP
 *
 * Copyright (C) 2019-
 * For license information, please see the LICENSE file.
 */

#ifndef ZEND_SCOUTAPM_H
#define ZEND_SCOUTAPM_H

#include "config.h"

#include "php.h"
#include <zend_extensions.h>
#include <zend_compile.h>
#include <zend_exceptions.h>
#include "ext/standard/php_var.h"

#include "scout_recording.h"
#include "scout_internal_handlers.h"
#include "scout_execute_ex.h"

#define PHP_SCOUTAPM_NAME "scoutapm"
#define PHP_SCOUTAPM_VERSION "1.3.0"

/* Extreme amounts of debugging, set to 1 to enable it and `make clean && make` (tests will fail...) */
#define SCOUT_APM_EXT_DEBUGGING 0

#if PHP_VERSION_ID > 80000
#define SCOUTAPM_INSTRUMENT_USING_OBSERVER_API 1
#else
#define SCOUTAPM_INSTRUMENT_USING_OBSERVER_API 0
#endif

PHP_FUNCTION(scoutapm_get_calls);
PHP_FUNCTION(scoutapm_list_instrumented_functions);

/* These are the "module globals". In non-ZTS mode, they're just regular variables, but means in ZTS mode they get handled properly */
ZEND_BEGIN_MODULE_GLOBALS(scoutapm)
    zend_bool handlers_set;
    zend_long observed_stack_frames_count;
    scoutapm_stack_frame *observed_stack_frames;
    zend_long disconnected_call_argument_store_count;
    scoutapm_disconnected_call_argument_store *disconnected_call_argument_store;
    scoutapm_instrumented_function instrumented_function_names[MAX_INSTRUMENTED_FUNCTIONS];
    int num_instrumented_functions;
    int currently_instrumenting;
#if SCOUTAPM_INSTRUMENT_USING_OBSERVER_API == 1
    double observer_api_start_time;
#endif //SCOUTAPM_INSTRUMENT_USING_OBSERVER_API
ZEND_END_MODULE_GLOBALS(scoutapm)

/* Accessor for "module globals" for non-ZTS and ZTS modes. */
#ifdef ZTS
#define SCOUTAPM_G(v) ZEND_MODULE_GLOBALS_ACCESSOR(scoutapm, v)
#else
#define SCOUTAPM_G(v) (scoutapm_globals.v)
#endif

/* zif_handler is not always defined, so define this roughly equivalent */
#if PHP_VERSION_ID < 70200
typedef void (*zif_handler)(INTERNAL_FUNCTION_PARAMETERS);
#endif

/* Sometimes this isn't defined, so define it in that case */
#ifndef ZEND_PARSE_PARAMETERS_NONE
#define ZEND_PARSE_PARAMETERS_NONE() if (zend_parse_parameters_none() != SUCCESS) { return; }
#endif

/* The debugging toggle allows us to output unnecessary amounts of information. Because it's a preprocessor flag, this stuff is compiled out */
#if SCOUT_APM_EXT_DEBUGGING == 1
#define SCOUTAPM_DEBUG_MESSAGE(x, ...) php_printf(x, ##__VA_ARGS__)
#else
#define SCOUTAPM_DEBUG_MESSAGE(...) /**/
#endif

/* Shortcut defined to allow using a `char *` with snprintf - determine the size first, allocate, then snprintf */
#define DYNAMIC_MALLOC_SPRINTF(destString, sizeNeeded, fmt, ...) \
    sizeNeeded = snprintf(NULL, 0, fmt, ##__VA_ARGS__) + 1; \
    destString = (char*)malloc(sizeNeeded); \
    snprintf(destString, sizeNeeded, fmt, ##__VA_ARGS__)

/* these are the string keys used in scoutapm_get_calls associative array return value */
#define SCOUT_GET_CALLS_KEY_FUNCTION "function"
#define SCOUT_GET_CALLS_KEY_ENTERED "entered"
#define SCOUT_GET_CALLS_KEY_EXITED "exited"
#define SCOUT_GET_CALLS_KEY_TIME_TAKEN "time_taken"
#define SCOUT_GET_CALLS_KEY_ARGV "argv"

/* stored argument wrapper constants */
#define SCOUT_WRAPPER_TYPE_CURL "curl_exec"
#define SCOUT_WRAPPER_TYPE_FILE "file"

#endif /* ZEND_SCOUTAPM_H */
