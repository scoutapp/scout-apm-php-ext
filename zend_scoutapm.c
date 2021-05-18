/*
 * Scout APM extension for PHP
 *
 * Copyright (C) 2019-
 * For license information, please see the LICENSE file.
 */

#include "zend_scoutapm.h"
#include "ext/standard/info.h"

static PHP_GINIT_FUNCTION(scoutapm);
static PHP_RINIT_FUNCTION(scoutapm);
static PHP_RSHUTDOWN_FUNCTION(scoutapm);
static PHP_MINIT_FUNCTION(scoutapm);
static PHP_MSHUTDOWN_FUNCTION(scoutapm);
static int zend_scoutapm_startup(zend_extension*);

extern void allocate_stack_frames_for_request();
extern void free_observed_stack_frames();
extern void free_recorded_call_arguments();
extern int setup_recording_for_internal_handlers();
extern int setup_functions_for_zend_execute_ex();
extern int setup_functions_for_observer_api();
extern void register_scout_execute_ex();
extern void deregister_scout_execute_ex();
extern void register_scout_observer();

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

    allocate_stack_frames_for_request();

    if (SCOUTAPM_G(handlers_set) != 1) {
        SCOUTAPM_DEBUG_MESSAGE("Overriding function handlers.\n");

        if (setup_functions_for_zend_execute_ex() == FAILURE) {
            return FAILURE;
        }

        if (setup_recording_for_internal_handlers() == FAILURE) {
            return FAILURE;
        }

        if (setup_functions_for_observer_api() == FAILURE) {
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
    free_observed_stack_frames();
    free_recorded_call_arguments();

    return SUCCESS;
}

static PHP_MINIT_FUNCTION(scoutapm)
{
    register_scout_observer();
    register_scout_execute_ex();

    return SUCCESS;
}

static PHP_MSHUTDOWN_FUNCTION(scoutapm)
{
    deregister_scout_execute_ex();

    return SUCCESS;
}
