/*
 * Scout APM extension for PHP
 *
 * Copyright (C) 2019-
 * For license information, please see the LICENSE file.
 */

#include "zend_scoutapm.h"

#if HAVE_SCOUT_CURL
#include <curl/curl.h>
#include "scout_extern.h"

#define ASSIGN_CURL_HANDLE_CLASS_ENTRY \
    zend_class_entry *curl_ce; \
    curl_ce = zend_hash_str_find_ptr(CG(class_table), "curlhandle", sizeof("curlhandle") - 1);

ZEND_NAMED_FUNCTION(scoutapm_curl_setopt_handler)
{
    zval *zid, *zvalue;
    zend_long options;
    const char *passthru_function_name;

#if PHP_MAJOR_VERSION >= 8
    const char *class_instance_id;
    ASSIGN_CURL_HANDLE_CLASS_ENTRY
#else
    const char *resource_id;
#endif

    SCOUT_PASSTHRU_IF_ALREADY_INSTRUMENTING(passthru_function_name)

    ZEND_PARSE_PARAMETERS_START(3, 3)
#if PHP_MAJOR_VERSION >= 8
            Z_PARAM_OBJECT_OF_CLASS(zid, curl_ce)
#else
            Z_PARAM_RESOURCE(zid)
#endif
            Z_PARAM_LONG(options)
            Z_PARAM_ZVAL(zvalue)
    ZEND_PARSE_PARAMETERS_END();

    if (options == CURLOPT_URL) {
#if PHP_MAJOR_VERSION >= 8
        class_instance_id = unique_class_instance_id(zid);
        record_arguments_for_call(class_instance_id, 1, zvalue);
        free((void*) class_instance_id);
#else
        resource_id = unique_resource_id(SCOUT_WRAPPER_TYPE_CURL, zid);
        record_arguments_for_call(resource_id, 1, zvalue);
        free((void*) resource_id);
#endif
    }

    SCOUT_INTERNAL_FUNCTION_PASSTHRU(passthru_function_name);
}

ZEND_NAMED_FUNCTION(scoutapm_curl_exec_handler)
{
    int handler_index;
    double entered = scoutapm_microtime();
    zval *zid;
    const char *called_function;
    zend_long recorded_arguments_index;

#if PHP_MAJOR_VERSION >= 8
    const char *class_instance_id;
    ASSIGN_CURL_HANDLE_CLASS_ENTRY
#else
    const char *resource_id;
#endif

    SCOUT_PASSTHRU_IF_ALREADY_INSTRUMENTING(called_function)

    called_function = determine_function_name(execute_data);

    ZEND_PARSE_PARAMETERS_START(1, 1)
#if PHP_MAJOR_VERSION >= 8
        Z_PARAM_OBJECT_OF_CLASS(zid, curl_ce)
#else
        Z_PARAM_RESOURCE(zid)
#endif
    ZEND_PARSE_PARAMETERS_END();

    handler_index = handler_index_for_function(called_function);

#if PHP_MAJOR_VERSION >= 8
    class_instance_id = unique_class_instance_id(zid);
    recorded_arguments_index = find_index_for_recorded_arguments(class_instance_id);
    free((void*) class_instance_id);
#else
    resource_id = unique_resource_id(SCOUT_WRAPPER_TYPE_CURL, zid);
    recorded_arguments_index = find_index_for_recorded_arguments(resource_id);
    free((void*) resource_id);
#endif

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
#endif
