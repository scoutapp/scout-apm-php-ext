/*
 * Scout APM extension for PHP
 *
 * Copyright (C) 2021
 * For license information, please see the LICENSE file.
 */

#include "zend_scoutapm.h"
#include "scout_extern.h"
#include "Zend/zend_observer.h"

int setup_functions_for_observer_api()
{
#if SCOUTAPM_INSTRUMENT_USING_OBSERVER_API == 1
    ADD_FUNCTION_TO_INSTRUMENTATION_SAFE_CATCH("Predis\\Client->__call");
#endif

    return SUCCESS;
}

#if SCOUTAPM_INSTRUMENT_USING_OBSERVER_API == 1

static void scout_observer_begin(zend_execute_data *execute_data)
{
    if (SCOUTAPM_G(currently_instrumenting)) {
        return;
    }

    SCOUTAPM_G(observer_api_start_time) = scoutapm_microtime();
    SCOUTAPM_G(currently_instrumenting) = 1;
}

static void scout_observer_end(zend_execute_data *execute_data, zval *return_value)
{
    const char *function_name;

    if (SCOUTAPM_G(currently_instrumenting) != 1
        || SCOUTAPM_G(observer_api_start_time) <= 0) {
        return; // Possibly a weird situation? Not sure how we could get into this state
    }

    function_name = determine_function_name(execute_data);

    // @todo argc/argv

    // @todo if __call (maybe __callStatic too?) magic method, use the $method name as function name
    record_observed_stack_frame(function_name, SCOUTAPM_G(observer_api_start_time), scoutapm_microtime(), 0, NULL);
    SCOUTAPM_G(currently_instrumenting) = 0;
    SCOUTAPM_G(observer_api_start_time) = 0;
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

    if (should_be_instrumented(function_name) == 0) {
        return handlers;
    }

    handlers.begin = scout_observer_begin;
    handlers.end = scout_observer_end;
    return handlers;
}
#endif

void register_scout_observer()
{
#if SCOUTAPM_INSTRUMENT_USING_OBSERVER_API == 1
    zend_observer_fcall_register(scout_observer_api_register);
#endif
}
