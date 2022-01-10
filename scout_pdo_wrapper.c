/*
 * Scout APM extension for PHP
 *
 * Copyright (C) 2019-
 * For license information, please see the LICENSE file.
 */

#include "zend_scoutapm.h"
#include "scout_extern.h"

ZEND_NAMED_FUNCTION(scoutapm_pdo_prepare_handler)
{
    zval *statement;
    const char *passthru_function_name, *class_instance_id;

    SCOUT_PASSTHRU_IF_ALREADY_INSTRUMENTING(passthru_function_name)

    ZEND_PARSE_PARAMETERS_START(1, 10)
        Z_PARAM_ZVAL(statement)
    SCOUT_ZEND_PARSE_PARAMETERS_END();

    SCOUT_INTERNAL_FUNCTION_PASSTHRU(passthru_function_name);

    if (Z_TYPE_P(return_value) != IS_OBJECT) {
        return;
    }

    class_instance_id = unique_class_instance_id(return_value);
    record_arguments_for_call(class_instance_id, 1, statement);
    free((void*) class_instance_id);
}

ZEND_NAMED_FUNCTION(scoutapm_pdostatement_execute_handler)
{
    int handler_index;
    double entered = scoutapm_microtime();
    const char *called_function, *class_instance_id;
    zend_long recorded_arguments_index;

    SCOUT_PASSTHRU_IF_ALREADY_INSTRUMENTING(called_function)

    called_function = determine_function_name(execute_data);

    handler_index = handler_index_for_function(called_function);

    class_instance_id = unique_class_instance_id(getThis());
    recorded_arguments_index = find_index_for_recorded_arguments(class_instance_id);
    free((void*) class_instance_id);

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
