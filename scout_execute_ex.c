/*
 * Scout APM extension for PHP
 *
 * Copyright (C) 2021
 * For license information, please see the LICENSE file.
 */

#include "zend_scoutapm.h"
#include "scout_extern.h"

#if SCOUTAPM_INSTRUMENT_USING_OBSERVER_API == 0
void (*original_zend_execute_ex) (zend_execute_data *execute_data);
void (*original_zend_execute_internal) (zend_execute_data *execute_data, zval *return_value);
void scoutapm_execute_internal(zend_execute_data *execute_data, zval *return_value);
void scoutapm_execute_ex(zend_execute_data *execute_data);
#endif

int setup_functions_for_zend_execute_ex()
{
#if SCOUTAPM_INSTRUMENT_USING_OBSERVER_API == 0
    ADD_FUNCTION_TO_INSTRUMENTATION_SAFE_CATCH("Predis\\Client->get");
    ADD_FUNCTION_TO_INSTRUMENTATION_SAFE_CATCH("Predis\\Client->set");
    ADD_FUNCTION_TO_INSTRUMENTATION_SAFE_CATCH("Predis\\Client->del");
    ADD_FUNCTION_TO_INSTRUMENTATION_SAFE_CATCH("Predis\\Client->append");
    ADD_FUNCTION_TO_INSTRUMENTATION_SAFE_CATCH("Predis\\Client->incr");
    ADD_FUNCTION_TO_INSTRUMENTATION_SAFE_CATCH("Predis\\Client->decr");
    ADD_FUNCTION_TO_INSTRUMENTATION_SAFE_CATCH("Predis\\Client->incrBy");
    ADD_FUNCTION_TO_INSTRUMENTATION_SAFE_CATCH("Predis\\Client->decrBy");
#endif

    return SUCCESS;
}

void register_scout_execute_ex()
{
#if SCOUTAPM_INSTRUMENT_USING_OBSERVER_API == 0
    original_zend_execute_internal = zend_execute_internal;
    zend_execute_internal = scoutapm_execute_internal;

    original_zend_execute_ex = zend_execute_ex;
    zend_execute_ex = scoutapm_execute_ex;
#endif
}

void deregister_scout_execute_ex()
{
#if SCOUTAPM_INSTRUMENT_USING_OBSERVER_API == 0
    zend_execute_internal = original_zend_execute_internal;
    zend_execute_ex = original_zend_execute_ex;
#endif
}

#if SCOUTAPM_INSTRUMENT_USING_OBSERVER_API == 0

void scoutapm_execute_internal(zend_execute_data *execute_data, zval *return_value)
{
    const char *function_name;
    double entered = scoutapm_microtime();
    int argc;
    zval *argv = NULL;

    if (execute_data->func->common.function_name == NULL) {
        if (original_zend_execute_internal) {
            original_zend_execute_internal(execute_data, return_value);
        } else {
            execute_internal(execute_data, return_value);
        }
        return;
    }

    function_name = determine_function_name(execute_data);

    if (should_be_instrumented(function_name, NULL) == 0 || SCOUTAPM_G(currently_instrumenting) == 1) {
        if (original_zend_execute_internal) {
            original_zend_execute_internal(execute_data, return_value);
        } else {
            execute_internal(execute_data, return_value);
        }
        return;
    }

    SCOUTAPM_G(currently_instrumenting) = 1;

    ZEND_PARSE_PARAMETERS_START(0, -1)
            Z_PARAM_VARIADIC(' ', argv, argc)
    ZEND_PARSE_PARAMETERS_END();

    if (original_zend_execute_internal) {
        original_zend_execute_internal(execute_data, return_value);
    } else {
        execute_internal(execute_data, return_value);
    }

    record_observed_stack_frame(function_name, entered, scoutapm_microtime(), argc, argv);
    SCOUTAPM_G(currently_instrumenting) = 0;
}

void scoutapm_execute_ex(zend_execute_data *execute_data)
{
    const char *function_name;
    double entered = scoutapm_microtime();
    int argc;
    zval *argv = NULL;

    if (execute_data->func->common.function_name == NULL) {
        original_zend_execute_ex(execute_data);
        return;
    }

    function_name = determine_function_name(execute_data);

    if (should_be_instrumented(function_name, NULL) == 0 || SCOUTAPM_G(currently_instrumenting) == 1) {
        original_zend_execute_ex(execute_data);
        return;
    }

    SCOUTAPM_G(currently_instrumenting) = 1;

    ZEND_PARSE_PARAMETERS_START(0, -1)
            Z_PARAM_VARIADIC(' ', argv, argc)
    ZEND_PARSE_PARAMETERS_END();

    original_zend_execute_ex(execute_data);

    record_observed_stack_frame(function_name, entered, scoutapm_microtime(), argc, argv);
    SCOUTAPM_G(currently_instrumenting) = 0;
}

#endif /* SCOUTAPM_INSTRUMENT_USING_OBSERVER_API */
