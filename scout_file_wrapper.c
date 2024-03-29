/*
 * Scout APM extension for PHP
 *
 * Copyright (C) 2019-
 * For license information, please see the LICENSE file.
 */

#include "zend_scoutapm.h"
#include "scout_extern.h"

ZEND_NAMED_FUNCTION(scoutapm_fopen_handler)
{
    zend_string *filename, *mode;
    zval argv[2];
    const char *passthru_function_name, *resource_id;

    SCOUT_PASSTHRU_IF_ALREADY_INSTRUMENTING(passthru_function_name)

    ZEND_PARSE_PARAMETERS_START(2, 4)
            Z_PARAM_STR(filename)
            Z_PARAM_STR(mode)
    SCOUT_ZEND_PARSE_PARAMETERS_END();

    ZVAL_STR(&argv[0], filename);
    ZVAL_STR(&argv[1], mode);

    SCOUT_INTERNAL_FUNCTION_PASSTHRU(passthru_function_name);

    if (Z_TYPE_P(return_value) == IS_RESOURCE) {
        resource_id = unique_resource_id(SCOUT_WRAPPER_TYPE_FILE, return_value);
        record_arguments_for_call(resource_id, 2, argv);
        free((void*) resource_id);
    }
}

ZEND_NAMED_FUNCTION(scoutapm_fread_handler)
{
    int handler_index;
    double entered = scoutapm_microtime();
    zval *resource_id;
    const char *called_function, *str_resource_id;
    zend_long recorded_arguments_index;

    SCOUT_PASSTHRU_IF_ALREADY_INSTRUMENTING(called_function)

    called_function = determine_function_name(execute_data);

    ZEND_PARSE_PARAMETERS_START(1, 10)
            Z_PARAM_RESOURCE(resource_id)
    SCOUT_ZEND_PARSE_PARAMETERS_END();

    handler_index = handler_index_for_function(called_function);

    str_resource_id = unique_resource_id(SCOUT_WRAPPER_TYPE_FILE, resource_id);
    recorded_arguments_index = find_index_for_recorded_arguments(str_resource_id);
    free((void*) str_resource_id);

    if (recorded_arguments_index < 0) {
        free((void*) called_function);
        scoutapm_default_handler(INTERNAL_FUNCTION_PARAM_PASSTHRU);
        return;
    }

    original_handlers[handler_index](INTERNAL_FUNCTION_PARAM_PASSTHRU);

    record_observed_stack_frame(
        called_function,
        entered,
        scoutapm_microtime(),
        SCOUTAPM_G(disconnected_call_argument_store)[recorded_arguments_index].argc,
        SCOUTAPM_G(disconnected_call_argument_store)[recorded_arguments_index].argv
    );
    free((void*) called_function);
}

ZEND_NAMED_FUNCTION(scoutapm_fwrite_handler)
{
    int handler_index;
    double entered = scoutapm_microtime();
    zval *resource_id;
    const char *called_function, *str_resource_id;
    zend_long recorded_arguments_index;

    SCOUT_PASSTHRU_IF_ALREADY_INSTRUMENTING(called_function)

    called_function = determine_function_name(execute_data);

    ZEND_PARSE_PARAMETERS_START(1, 10)
            Z_PARAM_RESOURCE(resource_id)
    SCOUT_ZEND_PARSE_PARAMETERS_END();

    handler_index = handler_index_for_function(called_function);

    str_resource_id = unique_resource_id(SCOUT_WRAPPER_TYPE_FILE, resource_id);
    recorded_arguments_index = find_index_for_recorded_arguments(str_resource_id);
    free((void*) str_resource_id);

    if (recorded_arguments_index < 0) {
        free((void*) called_function);
        scoutapm_default_handler(INTERNAL_FUNCTION_PARAM_PASSTHRU);
        return;
    }

    original_handlers[handler_index](INTERNAL_FUNCTION_PARAM_PASSTHRU);

    record_observed_stack_frame(
        called_function,
        entered,
        scoutapm_microtime(),
        SCOUTAPM_G(disconnected_call_argument_store)[recorded_arguments_index].argc,
        SCOUTAPM_G(disconnected_call_argument_store)[recorded_arguments_index].argv
    );
    free((void*) called_function);
}
