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

    SCOUT_PASSTHRU_IF_ALREADY_INSTRUMENTING()

    ZEND_PARSE_PARAMETERS_START(1, 10)
        Z_PARAM_ZVAL(statement)
    ZEND_PARSE_PARAMETERS_END();

    SCOUT_INTERNAL_FUNCTION_PASSTHRU();

    if (Z_TYPE_P(return_value) != IS_OBJECT) {
        return;
    }

    record_arguments_for_call(unique_class_instance_id(return_value), 1, statement);
}

ZEND_NAMED_FUNCTION(scoutapm_pdostatement_execute_handler)
{
    int handler_index;
    double entered = scoutapm_microtime();
    const char *called_function;
    zend_long recorded_arguments_index;

    SCOUT_PASSTHRU_IF_ALREADY_INSTRUMENTING()

    called_function = determine_function_name(execute_data);

    handler_index = handler_index_for_function(called_function);

    recorded_arguments_index = find_index_for_recorded_arguments(unique_class_instance_id(getThis()));

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
