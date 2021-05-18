/*
 * Scout APM extension for PHP
 *
 * Copyright (C) 2021
 * For license information, please see the LICENSE file.
 */

#include "zend_scoutapm.h"
#include "scout_extern.h"

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

void allocate_stack_frames_for_request()
{
    SCOUTAPM_DEBUG_MESSAGE("Initialising stacks...");
    SCOUTAPM_G(observed_stack_frames_count) = 0;
    SCOUTAPM_G(observed_stack_frames) = calloc(0, sizeof(scoutapm_stack_frame));
    SCOUTAPM_G(disconnected_call_argument_store_count) = 0;
    SCOUTAPM_G(disconnected_call_argument_store) = calloc(0, sizeof(scoutapm_disconnected_call_argument_store));
    SCOUTAPM_DEBUG_MESSAGE("Stacks made\n");

    SCOUTAPM_G(currently_instrumenting) = 0;
    SCOUTAPM_G(num_instrumented_functions) = 0;
}

void free_observed_stack_frames()
{
    int i, j;

    SCOUTAPM_DEBUG_MESSAGE("Freeing stacks... ");

    for (i = 0; i < SCOUTAPM_G(observed_stack_frames_count); i++) {
        for (j = 0; j < SCOUTAPM_G(observed_stack_frames)[i].argc; j++) {
            zval_ptr_dtor(&(SCOUTAPM_G(observed_stack_frames)[i].argv[j]));
        }
        free(SCOUTAPM_G(observed_stack_frames)[i].argv);
        free((void*)SCOUTAPM_G(observed_stack_frames)[i].function_name);
    }

    if (SCOUTAPM_G(observed_stack_frames)) {
        free(SCOUTAPM_G(observed_stack_frames));
    }
    SCOUTAPM_G(observed_stack_frames_count) = 0;

    SCOUTAPM_DEBUG_MESSAGE("Stacks freed\n");
}

void free_recorded_call_arguments()
{
    zend_long i, j;

    for (i = 0; i < SCOUTAPM_G(disconnected_call_argument_store_count); i++) {
        free((void*)SCOUTAPM_G(disconnected_call_argument_store)[i].reference);

        for (j = 0; j < SCOUTAPM_G(disconnected_call_argument_store)[i].argc; j++) {
            zval_ptr_dtor(&SCOUTAPM_G(disconnected_call_argument_store)[i].argv[j]);
        }
        free(SCOUTAPM_G(disconnected_call_argument_store)[i].argv);
    }

    free(SCOUTAPM_G(disconnected_call_argument_store));
    SCOUTAPM_G(disconnected_call_argument_store_count) = 0;
}

void record_arguments_for_call(const char *call_reference, int argc, zval *argv)
{
    zend_long i = 0;

    SCOUTAPM_G(disconnected_call_argument_store) = realloc(
        SCOUTAPM_G(disconnected_call_argument_store),
        (SCOUTAPM_G(disconnected_call_argument_store_count)+1) * sizeof(scoutapm_disconnected_call_argument_store)
    );

    SCOUTAPM_G(disconnected_call_argument_store)[SCOUTAPM_G(disconnected_call_argument_store_count)].reference = strdup(call_reference);
    SCOUTAPM_G(disconnected_call_argument_store)[SCOUTAPM_G(disconnected_call_argument_store_count)].argc = argc;
    SCOUTAPM_G(disconnected_call_argument_store)[SCOUTAPM_G(disconnected_call_argument_store_count)].argv = calloc(argc, sizeof(zval));

    for(; i < argc; i++) {
        safely_copy_argument_zval_as_scalar(
            &argv[i],
            &SCOUTAPM_G(disconnected_call_argument_store)[SCOUTAPM_G(disconnected_call_argument_store_count)].argv[i]
        );
    }

    SCOUTAPM_G(disconnected_call_argument_store_count)++;
}

zend_long find_index_for_recorded_arguments(const char *call_reference)
{
    zend_long i = 0;
    for (; i < SCOUTAPM_G(disconnected_call_argument_store_count); i++) {
        if (SCOUTAPM_G(disconnected_call_argument_store)[i].reference
            && strcasecmp(
            SCOUTAPM_G(disconnected_call_argument_store)[i].reference,
            call_reference
        ) == 0
            ) {
            return i;
        }
    }

#if SCOUT_APM_EXT_DEBUGGING == 1
    php_error_docref("", E_NOTICE, "ScoutAPM could not determine arguments for this call");
#endif

    return -1;
}

/*
 * Helper function to handle memory allocation for recorded stack frames. Called each time a function has completed
 * that we're interested in.
 */
void record_observed_stack_frame(const char *function_name, double microtime_entered, double microtime_exited, int argc, zval *argv)
{
    int i;

    if (argc > 0) {
        SCOUTAPM_DEBUG_MESSAGE("Adding observed stack frame for %s (%s) ... ", function_name, Z_STRVAL(argv[0]));
    } else {
        SCOUTAPM_DEBUG_MESSAGE("Adding observed stack frame for %s ... ", function_name);
    }
    SCOUTAPM_G(observed_stack_frames) = realloc(
        SCOUTAPM_G(observed_stack_frames),
        (SCOUTAPM_G(observed_stack_frames_count)+1) * sizeof(scoutapm_stack_frame)
    );

    SCOUTAPM_G(observed_stack_frames)[SCOUTAPM_G(observed_stack_frames_count)].function_name = strdup(function_name);
    SCOUTAPM_G(observed_stack_frames)[SCOUTAPM_G(observed_stack_frames_count)].entered = microtime_entered;
    SCOUTAPM_G(observed_stack_frames)[SCOUTAPM_G(observed_stack_frames_count)].exited = microtime_exited;
    SCOUTAPM_G(observed_stack_frames)[SCOUTAPM_G(observed_stack_frames_count)].argc = argc;
    SCOUTAPM_G(observed_stack_frames)[SCOUTAPM_G(observed_stack_frames_count)].argv = calloc(argc, sizeof(zval));

    for (i = 0; i < argc; i++) {
        safely_copy_argument_zval_as_scalar(
            &(argv[i]),
            &(SCOUTAPM_G(observed_stack_frames)[SCOUTAPM_G(observed_stack_frames_count)].argv[i])
        );
    }

    SCOUTAPM_G(observed_stack_frames_count)++;
    SCOUTAPM_DEBUG_MESSAGE("Done\n");
}
