/*
 * Scout APM extension for PHP
 *
 * Copyright (C) 2019-
 * For license information, please see the LICENSE file.
 */

#include "zend_scoutapm.h"
#include "scout_extern.h"

const char *unique_pdo_statement_id(zval *pdo_statement_instance)
{
    int len;
    char *ret;

    DYNAMIC_MALLOC_SPRINTF(ret, len,
        "pdo_class(%s)_instance(%d)",
        ZSTR_VAL(Z_OBJ_HT_P(pdo_statement_instance)->get_class_name(Z_OBJ_P(pdo_statement_instance))),
        Z_OBJ_HANDLE_P(pdo_statement_instance)
    );

    return ret;
}

ZEND_NAMED_FUNCTION(scoutapm_pdo_prepare_handler)
{
    zval *statement;

    ZEND_PARSE_PARAMETERS_START(1, 10)
        Z_PARAM_ZVAL(statement)
    ZEND_PARSE_PARAMETERS_END();

    SCOUT_INTERNAL_FUNCTION_PASSTHRU();

    record_arguments_for_call(unique_pdo_statement_id(return_value), 1, statement);
}

ZEND_NAMED_FUNCTION(scoutapm_pdostatement_execute_handler)
{
    int handler_index;
    double entered = scoutapm_microtime();
    const char *called_function;
    zend_long recorded_arguments_index;

    called_function = determine_function_name(execute_data);

    handler_index = handler_index_for_function(called_function);

    /* Practically speaking, this shouldn't happen as long as we defined the handlers properly */
    if (handler_index < 0) {
        zend_throw_exception(NULL, "ScoutAPM overwrote a handler for a function it didn't define a handler for", 0);
        return;
    }

    recorded_arguments_index = find_index_for_recorded_arguments(unique_pdo_statement_id(getThis()));

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
