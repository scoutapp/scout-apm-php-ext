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

    // @todo make call reference actually unique
    record_arguments_for_call("file", 2, argv);

    SCOUT_INTERNAL_FUNCTION_PASSTHRU();
}

ZEND_NAMED_FUNCTION(scoutapm_fread_handler)
{
    int handler_index;
    double entered = scoutapm_microtime();
    zval *resource_id;
    const char *called_function;
    zend_long recorded_arguments_index, bytes_to_read;

    called_function = determine_function_name(execute_data);

    ZEND_PARSE_PARAMETERS_START(2, 2)
            Z_PARAM_RESOURCE(resource_id)
            Z_PARAM_LONG(bytes_to_read)
    ZEND_PARSE_PARAMETERS_END();

    handler_index = handler_index_for_function(called_function);

    /* Practically speaking, this shouldn't happen as long as we defined the handlers properly */
    if (handler_index < 0) {
        zend_throw_exception(NULL, "ScoutAPM overwrote a handler for a function it didn't define a handler for", 0);
        return;
    }

    // @todo make call reference actually unique
    recorded_arguments_index = find_index_for_recorded_arguments("file");

    if (recorded_arguments_index < 0) {
        // @todo maybe log a warning? happens if we call fread without calling fopen...
        scoutapm_default_handler(INTERNAL_FUNCTION_PARAM_PASSTHRU);
        return;
    }

    // @todo segfault happens here if handler_index too high - https://github.com/scoutapp/scout-apm-php-ext/issues/41
    original_handlers[handler_index](INTERNAL_FUNCTION_PARAM_PASSTHRU);

    record_observed_stack_frame(
        called_function,
        entered,
        scoutapm_microtime(),
        SCOUTAPM_G(disconnected_call_argument_store)[recorded_arguments_index].argc,
        SCOUTAPM_G(disconnected_call_argument_store)[recorded_arguments_index].argv
    );
}
