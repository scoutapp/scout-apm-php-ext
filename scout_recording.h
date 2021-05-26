/*
 * Scout APM extension for PHP
 *
 * Copyright (C) 2021-
 * For license information, please see the LICENSE file.
 */

#ifndef SCOUTAPM_SCOUT_RECORDING_H
#define SCOUTAPM_SCOUT_RECORDING_H

#include <zend_types.h>

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

typedef struct _scoutapm_instrumented_function {
    const char *function_name;
    const char *magic_method_name;
} scoutapm_instrumented_function;

#define ADD_FUNCTION_TO_INSTRUMENTATION_SAFE_CATCH(function_name)                                             \
    zend_try {                                                                                                \
        add_function_to_instrumentation(function_name, NULL);                                                 \
    } zend_catch {                                                                                            \
        php_printf("ScoutAPM tried instrumenting '%s' - increase MAX_INSTRUMENTED_FUNCTIONS", function_name); \
        return FAILURE;                                                                                       \
    } zend_end_try()

/**
 * NOTE: magic method definitions are ONLY needed for Observer API instrumentation. The execute_data in zend_execte_ex
 * gives us the "called" method name (e.g. "MyObj->myMethod" instead of "MyObj->__call") so you don't need to use this
 * macro for defining methods in zend_execute_ex.
 */
#define ADD_MAGIC_FUNCTION_TO_INSTRUMENTATION_SAFE_CATCH(function_name, magic_method_function)                \
    zend_try {                                                                                                \
        add_function_to_instrumentation(function_name, magic_method_function);                                \
    } zend_catch {                                                                                            \
        php_printf("ScoutAPM tried instrumenting '%s' - increase MAX_INSTRUMENTED_FUNCTIONS", function_name); \
        return FAILURE;                                                                                       \
    } zend_end_try()

#endif //SCOUTAPM_SCOUT_RECORDING_H
