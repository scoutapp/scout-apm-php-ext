/*
 * Scout APM extension for PHP
 *
 * Copyright (C) 2019-
 * For license information, please see the LICENSE file.
 */

#include "zend_scoutapm.h"
#include "ext/standard/info.h"

void record_arguments_for_call(const char *call_reference, int argc, zval *argv);
zend_long find_index_for_recorded_arguments(const char *call_reference);
void record_observed_stack_frame(const char *function_name, double microtime_entered, double microtime_exited, int argc, zval *argv);
int handler_index_for_function(const char *function_to_lookup);

static PHP_GINIT_FUNCTION(scoutapm);
static PHP_RINIT_FUNCTION(scoutapm);
static PHP_RSHUTDOWN_FUNCTION(scoutapm);
static PHP_MINIT_FUNCTION(scoutapm);
static PHP_MSHUTDOWN_FUNCTION(scoutapm);
static int zend_scoutapm_startup(zend_extension*);

#if SCOUTAPM_INSTRUMENT_USING_OBSERVER_API == 1
extern zend_observer_fcall_handlers scout_observer_api_register(zend_execute_data *execute_data);
#else
static void (*original_zend_execute_ex) (zend_execute_data *execute_data);
static void (*original_zend_execute_internal) (zend_execute_data *execute_data, zval *return_value);
void scoutapm_execute_internal(zend_execute_data *execute_data, zval *return_value);
void scoutapm_execute_ex(zend_execute_data *execute_data);
#endif

extern int should_be_instrumented(const char *function_name);
extern const char* determine_function_name(zend_execute_data *execute_data);
extern double scoutapm_microtime();
extern void free_recorded_call_arguments();
extern int unchecked_handler_index_for_function(const char *function_to_lookup);
extern int setup_recording_for_functions();

extern indexed_handler_lookup handler_lookup[];
extern const int handler_lookup_size;
extern zif_handler original_handlers[];

ZEND_DECLARE_MODULE_GLOBALS(scoutapm)

ZEND_BEGIN_ARG_INFO_EX(no_arguments, 0, 0, 0)
ZEND_END_ARG_INFO()

/* a PHP module defines what functions it exports */
static const zend_function_entry scoutapm_functions[] = {
    PHP_FE(scoutapm_get_calls, no_arguments)
    PHP_FE(scoutapm_list_instrumented_functions, no_arguments)
    PHP_FE_END
};

PHP_MINFO_FUNCTION(scoutapm)
{
	php_info_print_table_start();
	php_info_print_table_header(2, "scoutapm support", "enabled");
	php_info_print_table_row(2, "Version", PHP_SCOUTAPM_VERSION);
#if HAVE_CURL
  #if HAVE_SCOUT_CURL
	php_info_print_table_row(2, "curl functions", "Yes");
  #else
	php_info_print_table_row(2, "curl functions", "Not instrumented");
  #endif
#else
	php_info_print_table_row(2, "curl functions", "No");
#endif
	php_info_print_table_row(2, "file functions", "Yes");
	php_info_print_table_row(2, "pdo functions", "Yes");
	php_info_print_table_end();
}

/* scoutapm_module_entry provides the metadata/information for PHP about this PHP module */
static zend_module_entry scoutapm_module_entry = {
    STANDARD_MODULE_HEADER,
    PHP_SCOUTAPM_NAME,
    scoutapm_functions,             /* function entries */
    PHP_MINIT(scoutapm),            /* module init */
    PHP_MSHUTDOWN(scoutapm),        /* module shutdown */
    PHP_RINIT(scoutapm),            /* request init */
    PHP_RSHUTDOWN(scoutapm),        /* request shutdown */
    PHP_MINFO(scoutapm),            /* module information */
    PHP_SCOUTAPM_VERSION,           /* module version */
    PHP_MODULE_GLOBALS(scoutapm),   /* module global variables */
    PHP_GINIT(scoutapm),            /* init global */
    NULL,
    NULL,
    STANDARD_MODULE_PROPERTIES_EX
};

/*
 * Do not export this module, so it cannot be registered with `extension=scoutapm.so` - must be `zend_extension=`
 * Instead, see `zend_scoutapm_startup` - we load the module there.
ZEND_GET_MODULE(scoutapm);
 */
#if defined(COMPILE_DL_SCOUTAPM) && defined(ZTS)
ZEND_TSRMLS_CACHE_DEFINE()
#endif

/* extension_version_info is used by PHP */
zend_extension_version_info extension_version_info = {
    ZEND_EXTENSION_API_NO,
    (char*) ZEND_EXTENSION_BUILD_ID
};

/* zend_extension_entry provides the metadata/information for PHP about this zend extension */
zend_extension zend_extension_entry = {
    (char*) PHP_SCOUTAPM_NAME,
    (char*) PHP_SCOUTAPM_VERSION,
    (char*) "Scout APM",
    (char*) "https://scoutapm.com/",
    (char*) "Copyright 2019",
    zend_scoutapm_startup,  /* extension startup */
    NULL,                   /* extension shutdown */
    NULL,                   /* request startup */
    NULL,                   /* request shutdown */
    NULL,                   /* message handler */
    NULL,                   /* compiler op_array_ahndler */
    NULL,                   /* VM statement_handler */
    NULL,                   /* VM fcall_begin_handler */
    NULL,                   /* VM_fcall_end_handler */
    NULL,                   /* compiler op_array_ctor */
    NULL,                   /* compiler op_array_dtor */
    STANDARD_ZEND_EXTENSION_PROPERTIES
};

/*
 * This is a hybrid "Zend Extension / PHP Module", and because the PHP module is not exported, we become responsible
 * for starting up the module. For more details on the PHP/Zend extension lifecycle, see:
 * http://www.phpinternalsbook.com/php7/extensions_design/zend_extensions.html#hybrid-extensions
 */
static int zend_scoutapm_startup(zend_extension *ze) {
    return zend_startup_module(&scoutapm_module_entry);
}

static PHP_GINIT_FUNCTION(scoutapm)
{
#if defined(COMPILE_DL_SCOUTAPM) && defined(ZTS)
    ZEND_TSRMLS_CACHE_UPDATE();
#endif
    memset(scoutapm_globals, 0, sizeof(zend_scoutapm_globals));
}

/*
 * Set up the handlers and stack. This is called at the start of each request to PHP.
 */
static PHP_RINIT_FUNCTION(scoutapm)
{
#if defined(COMPILE_DL_SCOUTAPM) && defined(ZTS)
    ZEND_TSRMLS_CACHE_UPDATE();
#endif

    SCOUTAPM_DEBUG_MESSAGE("Initialising stacks...");
    SCOUTAPM_G(observed_stack_frames_count) = 0;
    SCOUTAPM_G(observed_stack_frames) = calloc(0, sizeof(scoutapm_stack_frame));
    SCOUTAPM_G(disconnected_call_argument_store_count) = 0;
    SCOUTAPM_G(disconnected_call_argument_store) = calloc(0, sizeof(scoutapm_disconnected_call_argument_store));
    SCOUTAPM_DEBUG_MESSAGE("Stacks made\n");

    SCOUTAPM_G(currently_instrumenting) = 0;
    SCOUTAPM_G(num_instrumented_functions) = 0;

    if (SCOUTAPM_G(handlers_set) != 1) {
        SCOUTAPM_DEBUG_MESSAGE("Overriding function handlers.\n");

        if (setup_recording_for_functions() == FAILURE) {
            return FAILURE;
        }

        SCOUTAPM_G(handlers_set) = 1;
    } else {
        SCOUTAPM_DEBUG_MESSAGE("Handlers have already been set, skipping.\n");
    }

    return SUCCESS;
}

/*
 * At the end of the request, this function is responsible for freeing all memory we allocated. Note that if we forget
 * to free something, if PHP is compiled with --enable-debug, then we will see memory leaks.
 */
static PHP_RSHUTDOWN_FUNCTION(scoutapm)
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

    free_recorded_call_arguments();

    SCOUTAPM_DEBUG_MESSAGE("Stacks freed\n");

    return SUCCESS;
}

static PHP_MINIT_FUNCTION(scoutapm)
{
#if SCOUTAPM_INSTRUMENT_USING_OBSERVER_API == 1
    zend_observer_fcall_register(scout_observer_api_register);
#else
    original_zend_execute_internal = zend_execute_internal;
    zend_execute_internal = scoutapm_execute_internal;

    original_zend_execute_ex = zend_execute_ex;
    zend_execute_ex = scoutapm_execute_ex;
#endif

    return SUCCESS;
}

static PHP_MSHUTDOWN_FUNCTION(scoutapm)
{
#if SCOUTAPM_INSTRUMENT_USING_OBSERVER_API == 0
    zend_execute_internal = original_zend_execute_internal;
    zend_execute_ex = original_zend_execute_ex;
#endif

    return SUCCESS;
}

#if SCOUTAPM_INSTRUMENT_USING_OBSERVER_API == 0
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
#endif /* SCOUTAPM_INSTRUMENT_USING_OBSERVER_API == 0 */
