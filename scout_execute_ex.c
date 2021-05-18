/*
 * Scout APM extension for PHP
 *
 * Copyright (C) 2021
 * For license information, please see the LICENSE file.
 */

#include "zend_scoutapm.h"
#include "scout_extern.h"
#include "scout_execute_ex.h"

#if SCOUTAPM_INSTRUMENT_USING_OBSERVER_API == 1
extern zend_observer_fcall_handlers scout_observer_api_register(zend_execute_data *execute_data);
#else
void add_function_to_instrumentation(const char *function_name);
void (*original_zend_execute_ex) (zend_execute_data *execute_data);
void (*original_zend_execute_internal) (zend_execute_data *execute_data, zval *return_value);
void scoutapm_execute_internal(zend_execute_data *execute_data, zval *return_value);
void scoutapm_execute_ex(zend_execute_data *execute_data);
#endif

int setup_functions_for_zend_execute_ex()
{
#if SCOUTAPM_INSTRUMENT_USING_OBSERVER_API == 0
    ADD_FUNCTION_TO_INSTRUMENTATION_SAFE_CATCH("file_get_contents");
    ADD_FUNCTION_TO_INSTRUMENTATION_SAFE_CATCH("file_put_contents");
    ADD_FUNCTION_TO_INSTRUMENTATION_SAFE_CATCH("pdo->exec");
    ADD_FUNCTION_TO_INSTRUMENTATION_SAFE_CATCH("pdo->query");
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

void add_function_to_instrumentation(const char *function_name)
{
    if (SCOUTAPM_G(num_instrumented_functions) >= MAX_INSTRUMENTED_FUNCTIONS) {
        zend_throw_exception_ex(NULL, 0, "Unable to add instrumentation for function '%s' - MAX_INSTRUMENTED_FUNCTIONS of %d reached", function_name, MAX_INSTRUMENTED_FUNCTIONS);
        return;
    }

    SCOUTAPM_G(instrumented_function_names)[SCOUTAPM_G(num_instrumented_functions)] = strdup(function_name);
    SCOUTAPM_G(num_instrumented_functions)++;
}

int should_be_instrumented(const char *function_name)
{
    int i = 0;
    for (; i < SCOUTAPM_G(num_instrumented_functions); i++) {
        if (strcasecmp(function_name, SCOUTAPM_G(instrumented_function_names)[i]) == 0) {
            return 1;
        }
    }
    return 0;
}

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

    if (should_be_instrumented(function_name) == 0 || SCOUTAPM_G(currently_instrumenting) == 1) {
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

    if (should_be_instrumented(function_name) == 0 || SCOUTAPM_G(currently_instrumenting) == 1) {
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
