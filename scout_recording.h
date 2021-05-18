/*
 * Scout APM extension for PHP
 *
 * Copyright (C) 2021-
 * For license information, please see the LICENSE file.
 */

#ifndef SCOUTAPM_SCOUT_RECORDING_H
#define SCOUTAPM_SCOUT_RECORDING_H

#include <zend_types.h>

#define MAX_INSTRUMENTED_FUNCTIONS 100

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

#endif //SCOUTAPM_SCOUT_RECORDING_H
