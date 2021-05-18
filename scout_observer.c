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

void register_scout_observer()
{
#if SCOUTAPM_INSTRUMENT_USING_OBSERVER_API == 1
    zend_observer_fcall_register(scout_observer_api_register);
#endif
}
