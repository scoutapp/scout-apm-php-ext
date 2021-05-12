/*
 * Scout APM extension for PHP
 *
 * Copyright (C) 2019-
 * For license information, please see the LICENSE file.
 */

#include "zend_scoutapm.h"
#include "ext/standard/info.h"

double scoutapm_microtime();
void record_arguments_for_call(const char *call_reference, int argc, zval *argv);
zend_long find_index_for_recorded_arguments(const char *call_reference);
void record_observed_stack_frame(const char *function_name, double microtime_entered, double microtime_exited, int argc, zval *argv);
int handler_index_for_function(const char *function_to_lookup);
const char* determine_function_name(zend_execute_data *execute_data);

static PHP_GINIT_FUNCTION(scoutapm);
static PHP_RINIT_FUNCTION(scoutapm);
static PHP_RSHUTDOWN_FUNCTION(scoutapm);
static PHP_MINIT_FUNCTION(scoutapm);
static PHP_MSHUTDOWN_FUNCTION(scoutapm);
static int zend_scoutapm_startup(zend_extension*);
static void free_recorded_call_arguments();
static int unchecked_handler_index_for_function(const char *function_to_lookup);

#if SCOUTAPM_INSTRUMENT_USING_OBSERVER_API == 0
static void (*original_zend_execute_ex) (zend_execute_data *execute_data);
static void (*original_zend_execute_internal) (zend_execute_data *execute_data, zval *return_value);
void scoutapm_execute_internal(zend_execute_data *execute_data, zval *return_value);
void scoutapm_execute_ex(zend_execute_data *execute_data);
#endif

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
    { 0, "file_put_contents"},
    { 1, "curl_setopt"},
    { 2, "curl_exec"},
    { 3, "fopen"},
    { 4, "fread"},
    { 5, "fwrite"},
    { 6, "pdo->exec"},
    { 7, "pdo->query"},
    { 8, "pdo->prepare"},
    { 9, "pdostatement->execute"},
};

/* handlers count needs to be bigger than the number of handler_lookup entries */
#define ORIGINAL_HANDLERS_TO_ALLOCATE 20
zif_handler original_handlers[ORIGINAL_HANDLERS_TO_ALLOCATE] = {NULL};

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

    /* note - no strdup needed as we copy it in record_observed_stack_frame */
    called_function = determine_function_name(execute_data);

    ZEND_PARSE_PARAMETERS_START(0, -1)
        Z_PARAM_VARIADIC(' ', argv, argc)
    ZEND_PARSE_PARAMETERS_END();

    handler_index = handler_index_for_function(called_function);

    original_handlers[handler_index](INTERNAL_FUNCTION_PARAM_PASSTHRU);

    record_observed_stack_frame(called_function, entered, scoutapm_microtime(), argc, argv);
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
    zend_function *original_function;
    int handler_index;
    zend_class_entry *ce;

#if defined(COMPILE_DL_SCOUTAPM) && defined(ZTS)
    ZEND_TSRMLS_CACHE_UPDATE();
#endif

    SCOUTAPM_DEBUG_MESSAGE("Initialising stacks...");
    SCOUTAPM_G(observed_stack_frames_count) = 0;
    SCOUTAPM_G(observed_stack_frames) = calloc(0, sizeof(scoutapm_stack_frame));
    SCOUTAPM_G(disconnected_call_argument_store_count) = 0;
    SCOUTAPM_G(disconnected_call_argument_store) = calloc(0, sizeof(scoutapm_disconnected_call_argument_store));
    SCOUTAPM_DEBUG_MESSAGE("Stacks made\n");

    if (SCOUTAPM_G(handlers_set) != 1) {
        SCOUTAPM_DEBUG_MESSAGE("Overriding function handlers.\n");

        /* @todo make overloaded functions configurable? https://github.com/scoutapp/scout-apm-php-ext/issues/30 */
        SCOUT_OVERLOAD_FUNCTION("file_put_contents", scoutapm_default_handler)
#if HAVE_SCOUT_CURL
        SCOUT_OVERLOAD_FUNCTION("curl_setopt", scoutapm_curl_setopt_handler)
        SCOUT_OVERLOAD_FUNCTION("curl_exec", scoutapm_curl_exec_handler)
#endif
        SCOUT_OVERLOAD_FUNCTION("fopen", scoutapm_fopen_handler)
        SCOUT_OVERLOAD_FUNCTION("fwrite", scoutapm_fwrite_handler)
        SCOUT_OVERLOAD_FUNCTION("fread", scoutapm_fread_handler)
        SCOUT_OVERLOAD_METHOD("pdo", "exec", scoutapm_default_handler)
        SCOUT_OVERLOAD_METHOD("pdo", "query", scoutapm_default_handler)
        SCOUT_OVERLOAD_METHOD("pdo", "prepare", scoutapm_pdo_prepare_handler)
        SCOUT_OVERLOAD_METHOD("pdostatement", "execute", scoutapm_pdostatement_execute_handler)

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
#if SCOUTAPM_INSTRUMENT_USING_OBSERVER_API == 0
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

int should_be_instrumented(const char *function_name)
{
    // @todo make list dynamic and add-able-to
    const char *functions_to_be_instrumented[] = {
        "file_get_contents",
    };
    int size = 1, i = 0;
    for (; i < size; i++) {
        if (strcasecmp(function_name, functions_to_be_instrumented[i]) == 0) {
            return 1;
        }
    }
    return 0;
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

    if (should_be_instrumented(function_name) == 0) {
        if (original_zend_execute_internal) {
            original_zend_execute_internal(execute_data, return_value);
        } else {
            execute_internal(execute_data, return_value);
        }
        return;
    }

    ZEND_PARSE_PARAMETERS_START(0, -1)
            Z_PARAM_VARIADIC(' ', argv, argc)
    ZEND_PARSE_PARAMETERS_END();

    if (original_zend_execute_internal) {
        original_zend_execute_internal(execute_data, return_value);
    } else {
        execute_internal(execute_data, return_value);
    }

    record_observed_stack_frame(function_name, entered, scoutapm_microtime(), argc, argv);
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

    if (should_be_instrumented(function_name) == 0) {
        original_zend_execute_ex(execute_data);
        return;
    }

    ZEND_PARSE_PARAMETERS_START(0, -1)
            Z_PARAM_VARIADIC(' ', argv, argc)
    ZEND_PARSE_PARAMETERS_END();

    original_zend_execute_ex(execute_data);

    record_observed_stack_frame(function_name, entered, scoutapm_microtime(), argc, argv);
}
#endif /* SCOUTAPM_INSTRUMENT_USING_OBSERVER_API == 0 */

/*
 * Given some zend_execute_data, figure out what the function/method/static method is being called. The convention used
 * is `ClassName::methodName` for static methods, `ClassName->methodName` for instance methods, and `functionName` for
 * regular functions.
 */
const char* determine_function_name(zend_execute_data *execute_data)
{
    int len;
    char *ret;

    if (!execute_data->func) {
        return "<not a function call>";
    }

    if (execute_data->func->common.fn_flags & ZEND_ACC_STATIC) {
        DYNAMIC_MALLOC_SPRINTF(ret, len, "%s::%s",
            ZSTR_VAL(Z_CE(execute_data->This)->name),
            ZSTR_VAL(execute_data->func->common.function_name)
        );
        return ret;
    }

    if (Z_TYPE(execute_data->This) == IS_OBJECT) {
        DYNAMIC_MALLOC_SPRINTF(ret, len, "%s->%s",
            ZSTR_VAL(execute_data->func->common.scope->name),
            ZSTR_VAL(execute_data->func->common.function_name)
        );
        return ret;
    }

    return ZSTR_VAL(execute_data->func->common.function_name);
}

static int unchecked_handler_index_for_function(const char *function_to_lookup)
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

/*
 * Using gettimeofday, determine the time using microsecond precision, and return as a double.
 * @todo consider using HR monotonic time? https://github.com/scoutapp/scout-apm-php-ext/issues/23
 */
double scoutapm_microtime()
{
    struct timeval tp = {0};
    if (gettimeofday(&tp, NULL)) {
        zend_throw_exception_ex(zend_ce_exception, 0, "Could not call gettimeofday");
        return 0;
    }
    return (double)(tp.tv_sec + tp.tv_usec / 1000000.00);
}

static void free_recorded_call_arguments()
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
        ZVAL_COPY(
            &SCOUTAPM_G(disconnected_call_argument_store)[SCOUTAPM_G(disconnected_call_argument_store_count)].argv[i],
            &argv[i]
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

const char *zval_type_and_value_if_possible(zval *val)
{
    int len;
    char *ret;

reference_retry_point:
    switch (Z_TYPE_P(val)) {
        case IS_NULL:
            return "null";
        case IS_TRUE:
            return "bool(true)";
        case IS_FALSE:
            return "bool(false)";
        case IS_LONG:
            DYNAMIC_MALLOC_SPRINTF(ret, len,
                "int(%ld)",
                Z_LVAL_P(val)
            );
            return ret;
        case IS_DOUBLE:
            DYNAMIC_MALLOC_SPRINTF(ret, len,
                "float(%g)",
                Z_DVAL_P(val)
            );
            return ret;
        case IS_STRING:
            DYNAMIC_MALLOC_SPRINTF(ret, len,
                "string(%zd, \"%s\")",
                Z_STRLEN_P(val),
                Z_STRVAL_P(val)
            );
            return ret;
        case IS_RESOURCE:
            DYNAMIC_MALLOC_SPRINTF(ret, len,
                "resource(%d)",
                Z_RES_HANDLE_P(val)
            );
            return ret;
        case IS_ARRAY:
            return "array";
        case IS_OBJECT:
            DYNAMIC_MALLOC_SPRINTF(ret, len,
                "object(%s)",
                ZSTR_VAL(Z_OBJ_HT_P(val)->get_class_name(Z_OBJ_P(val)))
            );
            return ret;
        case IS_REFERENCE:
            val = Z_REFVAL_P(val);
            goto reference_retry_point;
        default:
            return "(unknown)";
    }
}

const char *unique_resource_id(const char *scout_wrapper_type, zval *resource_id)
{
    int len;
    char *ret;

    if (Z_TYPE_P(resource_id) != IS_RESOURCE) {
        zend_throw_exception_ex(NULL, 0, "ScoutAPM ext expected a resource, received: %s", zval_type_and_value_if_possible(resource_id));
        return "";
    }

    DYNAMIC_MALLOC_SPRINTF(ret, len,
        "%s_handle(%d)_type(%d)",
        scout_wrapper_type,
        Z_RES_HANDLE_P(resource_id),
        Z_RES_TYPE_P(resource_id)
    );
    return ret;
}

const char *unique_class_instance_id(zval *class_instance)
{
    int len;
    char *ret;

    if (Z_TYPE_P(class_instance) != IS_OBJECT) {
        zend_throw_exception_ex(NULL, 0, "ScoutAPM ext expected an object, received: %s", zval_type_and_value_if_possible(class_instance));
        return "";
    }

    DYNAMIC_MALLOC_SPRINTF(ret, len,
        "class(%s)_instance(%d)",
        ZSTR_VAL(Z_OBJ_HT_P(class_instance)->get_class_name(Z_OBJ_P(class_instance))),
        Z_OBJ_HANDLE_P(class_instance)
    );

    return ret;
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
        ZVAL_COPY(
            &(SCOUTAPM_G(observed_stack_frames)[SCOUTAPM_G(observed_stack_frames_count)].argv[i]),
            &(argv[i])
        );
    }

    SCOUTAPM_G(observed_stack_frames_count)++;
    SCOUTAPM_DEBUG_MESSAGE("Done\n");
}

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


/* {{{ proto array scoutapm_list_instrumented_functions()
   Fetch a list of functions that will be instrumented or monitored by the ScoutAPM extension. */
PHP_FUNCTION(scoutapm_list_instrumented_functions)
{
    int i, lookup_count = sizeof(handler_lookup) / sizeof(indexed_handler_lookup);

    // @todo also add zend_execute_ex list
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
}

/* }}} */
