/*
 * Scout APM extension for PHP
 *
 * Copyright (C) 2021
 * For license information, please see the LICENSE file.
 */

#include "zend_scoutapm.h"
#include "scout_extern.h"

#if SCOUTAPM_INSTRUMENT_USING_OBSERVER_API == 1
#include "Zend/zend_observer.h"
#endif

int setup_functions_for_observer_api()
{
#if SCOUTAPM_INSTRUMENT_USING_OBSERVER_API == 1
    ADD_MAGIC_FUNCTION_TO_INSTRUMENTATION_SAFE_CATCH("Predis\\Client->__call", "append");
    ADD_MAGIC_FUNCTION_TO_INSTRUMENTATION_SAFE_CATCH("Predis\\Client->__call", "decr");
    ADD_MAGIC_FUNCTION_TO_INSTRUMENTATION_SAFE_CATCH("Predis\\Client->__call", "decrBy");
    ADD_MAGIC_FUNCTION_TO_INSTRUMENTATION_SAFE_CATCH("Predis\\Client->__call", "get");
    ADD_MAGIC_FUNCTION_TO_INSTRUMENTATION_SAFE_CATCH("Predis\\Client->__call", "getBit");
    ADD_MAGIC_FUNCTION_TO_INSTRUMENTATION_SAFE_CATCH("Predis\\Client->__call", "getRange");
    ADD_MAGIC_FUNCTION_TO_INSTRUMENTATION_SAFE_CATCH("Predis\\Client->__call", "getSet");
    ADD_MAGIC_FUNCTION_TO_INSTRUMENTATION_SAFE_CATCH("Predis\\Client->__call", "incr");
    ADD_MAGIC_FUNCTION_TO_INSTRUMENTATION_SAFE_CATCH("Predis\\Client->__call", "incrBy");
    ADD_MAGIC_FUNCTION_TO_INSTRUMENTATION_SAFE_CATCH("Predis\\Client->__call", "mGet");
    ADD_MAGIC_FUNCTION_TO_INSTRUMENTATION_SAFE_CATCH("Predis\\Client->__call", "mSet");
    ADD_MAGIC_FUNCTION_TO_INSTRUMENTATION_SAFE_CATCH("Predis\\Client->__call", "mSetNx");
    ADD_MAGIC_FUNCTION_TO_INSTRUMENTATION_SAFE_CATCH("Predis\\Client->__call", "set");
    ADD_MAGIC_FUNCTION_TO_INSTRUMENTATION_SAFE_CATCH("Predis\\Client->__call", "setBit");
    ADD_MAGIC_FUNCTION_TO_INSTRUMENTATION_SAFE_CATCH("Predis\\Client->__call", "setEx");
    ADD_MAGIC_FUNCTION_TO_INSTRUMENTATION_SAFE_CATCH("Predis\\Client->__call", "pSetEx");
    ADD_MAGIC_FUNCTION_TO_INSTRUMENTATION_SAFE_CATCH("Predis\\Client->__call", "setNx");
    ADD_MAGIC_FUNCTION_TO_INSTRUMENTATION_SAFE_CATCH("Predis\\Client->__call", "setRange");
    ADD_MAGIC_FUNCTION_TO_INSTRUMENTATION_SAFE_CATCH("Predis\\Client->__call", "strlen");
    ADD_MAGIC_FUNCTION_TO_INSTRUMENTATION_SAFE_CATCH("Predis\\Client->__call", "del");
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
    char *function_name, *magic_function_name;
    size_t magic_function_name_len;
    int argc;
    zval *argv = NULL;

    if (SCOUTAPM_G(currently_instrumenting) != 1
        || SCOUTAPM_G(observer_api_start_time) <= 0) {
        return; // Possibly a weird situation? Not sure how we could get into this state
    }

    if (strcasecmp("__call", ZSTR_VAL(execute_data->func->common.function_name)) == 0) {
        // @todo would be nice to find a way to "unpack" the array here into argv
        ZEND_PARSE_PARAMETERS_START(1, -1)
            Z_PARAM_STRING(magic_function_name, magic_function_name_len)
            Z_PARAM_VARIADIC(' ', argv, argc)
        ZEND_PARSE_PARAMETERS_END();

        DYNAMIC_MALLOC_SPRINTF(function_name, magic_function_name_len, "%s->%s",
            ZSTR_VAL(execute_data->func->common.scope->name),
            magic_function_name
        );
    } else {
        function_name = (char*) determine_function_name(execute_data);

        ZEND_PARSE_PARAMETERS_START(0, -1)
            Z_PARAM_VARIADIC(' ', argv, argc)
        ZEND_PARSE_PARAMETERS_END();
    }

    record_observed_stack_frame(function_name, SCOUTAPM_G(observer_api_start_time), scoutapm_microtime(), argc, argv);
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

    // Can only resolve magic method name at call-time with Observer API, so pass NULL for the magic method name
    if (should_be_instrumented(function_name, NULL) == 0) {
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
