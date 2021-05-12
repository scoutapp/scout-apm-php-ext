/*
 * Scout APM extension for PHP
 *
 * Copyright (C) 2021
 * For license information, please see the LICENSE file.
 */

#include "zend_scoutapm.h"
#include "scout_extern.h"

extern void add_function_to_instrumentation(const char *function_name);
#if HAVE_SCOUT_CURL
extern ZEND_NAMED_FUNCTION(scoutapm_curl_setopt_handler);
extern ZEND_NAMED_FUNCTION(scoutapm_curl_exec_handler);
#endif
extern ZEND_NAMED_FUNCTION(scoutapm_fopen_handler);
extern ZEND_NAMED_FUNCTION(scoutapm_fread_handler);
extern ZEND_NAMED_FUNCTION(scoutapm_fwrite_handler);
extern ZEND_NAMED_FUNCTION(scoutapm_pdo_prepare_handler);
extern ZEND_NAMED_FUNCTION(scoutapm_pdostatement_execute_handler);

/* This is simply a map of function names to an index in original_handlers */
indexed_handler_lookup handler_lookup[] = {
    /* define each function we want to overload, which maps to an index in the `original_handlers` array */
    { 1, "curl_setopt"},
    { 2, "curl_exec"},
    { 3, "fopen"},
    { 4, "fread"},
    { 5, "fwrite"},
    { 6, "pdo->prepare"},
    { 7, "pdostatement->execute"},
};
const int handler_lookup_size = sizeof(handler_lookup);

/* handlers count needs to be bigger than the number of handler_lookup entries */
#define ORIGINAL_HANDLERS_TO_ALLOCATE 20
zif_handler original_handlers[ORIGINAL_HANDLERS_TO_ALLOCATE] = {NULL};

#define ADD_FUNCTION_TO_INSTRUMENTATION_SAFE_CATCH(function_name)                                             \
    zend_try {                                                                                                \
        add_function_to_instrumentation(function_name);                                                       \
    } zend_catch {                                                                                            \
        php_printf("ScoutAPM tried instrumenting '%s' - increase MAX_INSTRUMENTED_FUNCTIONS", function_name); \
        return FAILURE;                                                                                       \
    } zend_end_try()

int setup_recording_for_functions()
{
    zend_function *original_function;
    int handler_index;
    zend_class_entry *ce;

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

#if HAVE_SCOUT_CURL
    SCOUT_OVERLOAD_FUNCTION("curl_setopt", scoutapm_curl_setopt_handler)
    SCOUT_OVERLOAD_FUNCTION("curl_exec", scoutapm_curl_exec_handler)
#endif
    SCOUT_OVERLOAD_FUNCTION("fopen", scoutapm_fopen_handler)
    SCOUT_OVERLOAD_FUNCTION("fwrite", scoutapm_fwrite_handler)
    SCOUT_OVERLOAD_FUNCTION("fread", scoutapm_fread_handler)
    SCOUT_OVERLOAD_METHOD("pdo", "prepare", scoutapm_pdo_prepare_handler)
    SCOUT_OVERLOAD_METHOD("pdostatement", "execute", scoutapm_pdostatement_execute_handler)

    return SUCCESS;
}

/*
 * This handles most recorded calls by grabbing all arguments (we treat it as a variadic), finding the "original" handler
 * for the function and calling it. Once the function is called, record how long it took. Some "special" calls (e.g.
 * curl_exec) have special handling because the arguments to curl_exec don't indicate the URL, for example.
 */
ZEND_NAMED_FUNCTION(scoutapm_default_handler)
{
    int handler_index;
    double entered = scoutapm_microtime();
    int argc;
    zval *argv = NULL;
    const char *called_function;

    SCOUT_PASSTHRU_IF_ALREADY_INSTRUMENTING()

    /* note - no strdup needed as we copy it in record_observed_stack_frame */
    called_function = determine_function_name(execute_data);

    ZEND_PARSE_PARAMETERS_START(0, -1)
        Z_PARAM_VARIADIC(' ', argv, argc)
    ZEND_PARSE_PARAMETERS_END();

    handler_index = handler_index_for_function(called_function);

    original_handlers[handler_index](INTERNAL_FUNCTION_PARAM_PASSTHRU);

    record_observed_stack_frame(called_function, entered, scoutapm_microtime(), argc, argv);
}

int unchecked_handler_index_for_function(const char *function_to_lookup)
{
    int i = 0;
    const char *current = handler_lookup[i].function_name;

    while (current) {
        if (strcasecmp(current, function_to_lookup) == 0) {
            if (handler_lookup[i].index >= ORIGINAL_HANDLERS_TO_ALLOCATE) {
                /*
                 * Note: as this is done in startup, and a critical failure, php_error_docref or zend_throw_exception_ex
                 * aren't suitable here, as they are "exceptions"; therefore the most informative thing we can do is
                 * write a message directly, and return -1, capture this in the PHP_RINIT_FUNCTION and return FAILURE.
                 * The -1 should be checked in SCOUT_OVERLOAD_CLASS_ENTRY_FUNCTION and SCOUT_OVERLOAD_FUNCTION
                 */
                php_printf("ScoutAPM overwrote a handler for '%s' but but we did not allocate enough original_handlers", function_to_lookup);
                return -1;
            }

            return handler_lookup[i].index;
        }
        current = handler_lookup[++i].function_name;
    }

    /* Practically speaking, this shouldn't happen as long as we defined the handlers properly */
    zend_throw_exception_ex(NULL, 0, "ScoutAPM overwrote a handler for '%s' but did not have a handler lookup for it", function_to_lookup);
    return -1;
}

/*
 * In our handler_lookup, find what the "index" in our overridden handlers is for a particular function name
 */
int handler_index_for_function(const char *function_to_lookup)
{
    int handler_index = unchecked_handler_index_for_function(function_to_lookup);

    if (original_handlers[handler_index] == NULL) {
        zend_throw_exception_ex(NULL, 0, "ScoutAPM overwrote a handler for '%s' but the handler for index '%d' was not defined", function_to_lookup, handler_lookup[handler_index].index);
        return -1;
    }

    return handler_index;
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
