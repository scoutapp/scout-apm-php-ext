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
    zval *argv;

    ZEND_PARSE_PARAMETERS_START(2, 4)
            Z_PARAM_STR(filename)
            Z_PARAM_STR(mode)
    ZEND_PARSE_PARAMETERS_END();

    argv = calloc(2, sizeof(zval));
    ZVAL_STR(&argv[0], filename);
    ZVAL_STR(&argv[1], mode);

    SCOUT_INTERNAL_FUNCTION_PASSTHRU();

    record_arguments_for_call(unique_resource_id(SCOUT_WRAPPER_TYPE_FILE, return_value), 2, argv);
}

ZEND_NAMED_FUNCTION(scoutapm_fread_handler)
{
    int handler_index;
    double entered = scoutapm_microtime();
    zval *resource_id;
    const char *called_function;
    zend_long recorded_arguments_index;

    called_function = determine_function_name(execute_data);

    ZEND_PARSE_PARAMETERS_START(1, 10)
            Z_PARAM_RESOURCE(resource_id)
    ZEND_PARSE_PARAMETERS_END();

    handler_index = handler_index_for_function(called_function);

    recorded_arguments_index = find_index_for_recorded_arguments(unique_resource_id(SCOUT_WRAPPER_TYPE_FILE, resource_id));

    if (recorded_arguments_index < 0) {
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
}

ZEND_NAMED_FUNCTION(scoutapm_fwrite_handler)
{
    int handler_index;
    double entered = scoutapm_microtime();
    zval *resource_id;
    const char *called_function;
    zend_long recorded_arguments_index;

    called_function = determine_function_name(execute_data);

    ZEND_PARSE_PARAMETERS_START(1, 10)
            Z_PARAM_RESOURCE(resource_id)
    ZEND_PARSE_PARAMETERS_END();

    handler_index = handler_index_for_function(called_function);

    recorded_arguments_index = find_index_for_recorded_arguments(unique_resource_id(SCOUT_WRAPPER_TYPE_FILE, resource_id));

    if (recorded_arguments_index < 0) {
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
}
