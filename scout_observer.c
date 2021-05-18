/*
 * Scout APM extension for PHP
 *
 * Copyright (C) 2021
 * For license information, please see the LICENSE file.
 */

#include "zend_scoutapm.h"
#include "scout_extern.h"

#if SCOUTAPM_INSTRUMENT_USING_OBSERVER_API == 1

extern int should_be_instrumented(const char *function_name);

static void observer_begin(zend_execute_data *execute_data) {
    // @todo before
}

static void observer_end(zend_execute_data *execute_data, zval *return_value) {
    // @todo after
}

// Note: this is only called FIRST time each function is invoked (better that way)
zend_observer_fcall_handlers scout_observer_api_register(zend_execute_data *execute_data)
{
    const char *function_name;

    zend_observer_fcall_handlers handlers = {NULL, NULL};

    if (execute_data->func == NULL || execute_data->func->common.function_name == NULL) {
        return handlers;
    }

    function_name = determine_function_name(execute_data);

    // @todo magic methods become the function name here, unlike in zend_execute_ex
    if (should_be_instrumented(function_name) == 0) {
        return handlers;
    }

    handlers.begin = observer_begin;
    handlers.end = observer_end;
    return handlers; // I have handlers for this function
}
#endif
