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

#define SCOUTAPM_CURLOPT_URL "CURLOPT_URL"
#define SCOUTAPM_CURLOPT_POST "CURLOPT_POST"

#define SCOUTAPM_CURL_GET_CURLOPT_ADD_TO_ARGV(opt) \
    recorded_arguments_index = scout_curl_get_curlopt(zid, opt); \
    argv = realloc(argv, sizeof(zval) * ((unsigned long)argc + 1)); \
    if (recorded_arguments_index >= 0) { \
        argv[argc] = *SCOUTAPM_G(disconnected_call_argument_store)[recorded_arguments_index].argv; \
    } else { \
        zval null_value; \
        ZVAL_NULL(&null_value); \
        argv[argc] = null_value; \
    } \
    argc++;

static void scout_curl_store_curlopt(zval *curlHandle, const char *optionName, zval* optionValue)
{
#if PHP_MAJOR_VERSION >= 8
    char *class_instance_id = (char*)unique_class_instance_id(curlHandle);
    class_instance_id = realloc(class_instance_id, strlen(class_instance_id) + strlen(optionName) + 1);
    strcat(class_instance_id, optionName);

    record_arguments_for_call(class_instance_id, 1, optionValue);
    free((void*) class_instance_id);
#else
    char *resource_id = (char*)unique_resource_id(SCOUT_WRAPPER_TYPE_CURL, curlHandle);
    resource_id = realloc(resource_id, strlen(resource_id) + strlen(optionName) + 1);
    strcat(resource_id, optionName);

    record_arguments_for_call(resource_id, 1, optionValue);
    free((void*) resource_id);
#endif
}

static zend_long scout_curl_get_curlopt(zval *curlHandle, const char *optionName)
{
    zend_long recorded_arguments_index;

#if PHP_MAJOR_VERSION >= 8
    char *class_instance_id = (char*)unique_class_instance_id(curlHandle);
    class_instance_id = realloc(class_instance_id, strlen(class_instance_id) + strlen(optionName) + 1);
    strcat(class_instance_id, optionName);

    recorded_arguments_index = find_index_for_recorded_arguments(class_instance_id);
    free((void*) class_instance_id);
#else
    char *resource_id = (char*)unique_resource_id(SCOUT_WRAPPER_TYPE_CURL, curlHandle);
    resource_id = realloc(resource_id, strlen(resource_id) + strlen(optionName) + 1);
    strcat(resource_id, optionName);

    recorded_arguments_index = find_index_for_recorded_arguments(resource_id);
    free((void*) resource_id);
#endif

    return recorded_arguments_index;
}

ZEND_NAMED_FUNCTION(scoutapm_curl_setopt_handler)
{
    zval *zid, *zvalue;
    zend_long options;
    const char *passthru_function_name;

#if PHP_MAJOR_VERSION >= 8
    ASSIGN_CURL_HANDLE_CLASS_ENTRY
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
        scout_curl_store_curlopt(zid, SCOUTAPM_CURLOPT_URL, zvalue);
    }
    if (options == CURLOPT_POST) {
        scout_curl_store_curlopt(zid, SCOUTAPM_CURLOPT_POST, zvalue);
    }

    SCOUT_INTERNAL_FUNCTION_PASSTHRU(passthru_function_name);
}

ZEND_NAMED_FUNCTION(scoutapm_curl_exec_handler)
{
    int handler_index, argc = 0;
    double entered = scoutapm_microtime();
    zval *zid;
    const char *called_function;
    zend_long recorded_arguments_index;
    zval *argv = NULL;

#if PHP_MAJOR_VERSION >= 8
    ASSIGN_CURL_HANDLE_CLASS_ENTRY
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

    SCOUTAPM_CURL_GET_CURLOPT_ADD_TO_ARGV(SCOUTAPM_CURLOPT_URL);
    SCOUTAPM_CURL_GET_CURLOPT_ADD_TO_ARGV(SCOUTAPM_CURLOPT_POST);

    original_handlers[handler_index](INTERNAL_FUNCTION_PARAM_PASSTHRU);

    record_observed_stack_frame(
        called_function,
        entered,
        scoutapm_microtime(),
        argc,
        argv
    );
    free((void*) called_function);
}
#endif
