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

#define PHP_SCOUTAPM_NAME "scoutapm"
#define PHP_SCOUTAPM_VERSION "1.2.0"

/* Extreme amounts of debugging, set to 1 to enable it and `make clean && make` (tests will fail...) */
#define SCOUT_APM_EXT_DEBUGGING 0

PHP_FUNCTION(scoutapm_get_calls);
PHP_FUNCTION(scoutapm_list_instrumented_functions);

/* Describes information we store about a recorded stack frame */
typedef struct _scoutapm_stack_frame {
    const char *function_name;
    double entered;
    double exited;
    int argc;
    zval *argv;
} scoutapm_stack_frame;

typedef struct _scoutapm_disconnected_call_argument_store {
    const char *reference;
    int argc;
    zval *argv;
} scoutapm_disconnected_call_argument_store;

typedef struct _handler_lookup {
    int index;
    const char *function_name;
} indexed_handler_lookup;

/* These are the "module globals". In non-ZTS mode, they're just regular variables, but means in ZTS mode they get handled properly */
ZEND_BEGIN_MODULE_GLOBALS(scoutapm)
    zend_bool handlers_set;
    zend_long observed_stack_frames_count;
    scoutapm_stack_frame *observed_stack_frames;
    zend_long disconnected_call_argument_store_count;
    scoutapm_disconnected_call_argument_store *disconnected_call_argument_store;
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

/* overload a regular function by wrapping its handler with our own handler */
#define SCOUT_OVERLOAD_FUNCTION(function_name, handler_to_use) \
    original_function = zend_hash_str_find_ptr(EG(function_table), function_name, sizeof(function_name) - 1); \
    if (original_function != NULL) { \
        handler_index = unchecked_handler_index_for_function(function_name); \
        if (handler_index < 0) return FAILURE; \
        original_handlers[handler_index] = original_function->internal_function.handler; \
        original_function->internal_function.handler = handler_to_use; \
    }

/* Don't use this macro directly, use SCOUT_OVERLOAD_STATIC_METHOD or SCOUT_OVERLOAD_METHOD for consistency */
#define SCOUT_OVERLOAD_CLASS_ENTRY_FUNCTION(lowercase_class_name, instance_or_static, method_name, handler_to_use) \
    ce = zend_hash_str_find_ptr(CG(class_table), lowercase_class_name, sizeof(lowercase_class_name) - 1); \
    if (ce != NULL) { \
        original_function = zend_hash_str_find_ptr(&ce->function_table, method_name, sizeof(method_name)-1); \
        if (original_function != NULL) { \
            handler_index = unchecked_handler_index_for_function(lowercase_class_name instance_or_static method_name); \
            if (handler_index < 0) return FAILURE; \
            original_handlers[handler_index] = original_function->internal_function.handler; \
            original_function->internal_function.handler = handler_to_use; \
        } \
    }

/* overload a static class method by wrapping its handler with our own handler */
#define SCOUT_OVERLOAD_STATIC_METHOD(lowercase_class_name, method_name, handler_to_use) SCOUT_OVERLOAD_CLASS_ENTRY_FUNCTION(lowercase_class_name, "::", method_name, handler_to_use)

/* overload an instance class method by wrapping its handler with our own handler */
#define SCOUT_OVERLOAD_METHOD(lowercase_class_name, method_name, handler_to_use) SCOUT_OVERLOAD_CLASS_ENTRY_FUNCTION(lowercase_class_name, "->", method_name, handler_to_use)

#define SCOUT_INTERNAL_FUNCTION_PASSTHRU() original_handlers[handler_index_for_function(determine_function_name(execute_data))](INTERNAL_FUNCTION_PARAM_PASSTHRU)

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
