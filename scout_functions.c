/*
 * Scout APM extension for PHP
 *
 * Copyright (C) 2021
 * For license information, please see the LICENSE file.
 */

#include "zend_scoutapm.h"
#include "scout_extern.h"

/* {{{ proto array scoutapm_get_calls()
   Fetch all the recorded function or method calls recorded by the ScoutAPM extension. */
PHP_FUNCTION(scoutapm_get_calls)
{
    int i, j;
    zval item, arg_items, arg_item;
    ZEND_PARSE_PARAMETERS_NONE();

    SCOUTAPM_DEBUG_MESSAGE("scoutapm_get_calls: preparing return value... ");

    array_init(return_value);

    for (i = 0; i < SCOUTAPM_G(observed_stack_frames_count); i++) {
        array_init(&item);

        add_assoc_str_ex(
            &item,
            SCOUT_GET_CALLS_KEY_FUNCTION, strlen(SCOUT_GET_CALLS_KEY_FUNCTION),
            zend_string_init(SCOUTAPM_G(observed_stack_frames)[i].function_name, strlen(SCOUTAPM_G(observed_stack_frames)[i].function_name), 0)
        );

        add_assoc_double_ex(
            &item,
            SCOUT_GET_CALLS_KEY_ENTERED, strlen(SCOUT_GET_CALLS_KEY_ENTERED),
            SCOUTAPM_G(observed_stack_frames)[i].entered
        );

        add_assoc_double_ex(
            &item,
            SCOUT_GET_CALLS_KEY_EXITED, strlen(SCOUT_GET_CALLS_KEY_EXITED),
            SCOUTAPM_G(observed_stack_frames)[i].exited
        );

        /* Time taken is calculated here because float precision gets knocked off otherwise - so this is a useful metric */
        add_assoc_double_ex(
            &item,
            SCOUT_GET_CALLS_KEY_TIME_TAKEN, strlen(SCOUT_GET_CALLS_KEY_TIME_TAKEN),
            SCOUTAPM_G(observed_stack_frames)[i].exited - SCOUTAPM_G(observed_stack_frames)[i].entered
        );

        array_init(&arg_items);
        for (j = 0; j < SCOUTAPM_G(observed_stack_frames)[i].argc; j++) {
            /* Must copy the argument to a new zval, otherwise it gets freed and we get segfault. */
            ZVAL_COPY(&arg_item, &(SCOUTAPM_G(observed_stack_frames)[i].argv[j]));
            add_next_index_zval(&arg_items, &arg_item);
            zval_ptr_dtor(&(SCOUTAPM_G(observed_stack_frames)[i].argv[j]));
        }
        free(SCOUTAPM_G(observed_stack_frames)[i].argv);

        add_assoc_zval_ex(
            &item,
            SCOUT_GET_CALLS_KEY_ARGV, strlen(SCOUT_GET_CALLS_KEY_ARGV),
            &arg_items
        );

        add_next_index_zval(return_value, &item);

        free((void*)SCOUTAPM_G(observed_stack_frames)[i].function_name);
    }

    SCOUTAPM_G(observed_stack_frames) = realloc(SCOUTAPM_G(observed_stack_frames), 0);
    SCOUTAPM_G(observed_stack_frames_count) = 0;

    SCOUTAPM_DEBUG_MESSAGE("done.\n");
}
/* }}} */

/* {{{ proto array scoutapm_list_instrumented_functions()
   Fetch a list of functions that will be instrumented or monitored by the ScoutAPM extension. */
PHP_FUNCTION(scoutapm_list_instrumented_functions)
{
    int i, lookup_count = handler_lookup_size / sizeof(indexed_handler_lookup);

    array_init(return_value);

    for(i = 0; i < lookup_count; i++) {
        if (original_handlers[handler_lookup[i].index] == NULL) {
            continue;
        }

        add_next_index_stringl(
            return_value,
            handler_lookup[i].function_name,
            strlen(handler_lookup[i].function_name)
        );
    }

    for(i = 0; i < SCOUTAPM_G(num_instrumented_functions); i++) {
        add_next_index_stringl(
            return_value,
            SCOUTAPM_G(instrumented_function_names[i].function_name),
            strlen(SCOUTAPM_G(instrumented_function_names[i].function_name))
        );
    }
}
/* }}} */
