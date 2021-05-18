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

#define ADD_FUNCTION_TO_INSTRUMENTATION_SAFE_CATCH(function_name)                                             \
    zend_try {                                                                                                \
        add_function_to_instrumentation(function_name);                                                       \
    } zend_catch {                                                                                            \
        php_printf("ScoutAPM tried instrumenting '%s' - increase MAX_INSTRUMENTED_FUNCTIONS", function_name); \
        return FAILURE;                                                                                       \
    } zend_end_try()

#endif //SCOUTAPM_SCOUT_RECORDING_H
