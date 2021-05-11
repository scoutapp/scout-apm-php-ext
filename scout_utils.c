/*
 * Scout APM extension for PHP
 *
 * Copyright (C) 2021
 * For license information, please see the LICENSE file.
 */

#include "zend_scoutapm.h"
#include "scout_extern.h"

ZEND_NAMED_FUNCTION(scoutapm_default_handler);
void add_function_to_instrumentation(const char *function_name);
int should_be_instrumented(const char *function_name);
const char *zval_type_and_value_if_possible(zval *val);

void add_function_to_instrumentation(const char *function_name)
{
    if (SCOUTAPM_G(num_instrumented_functions) > MAX_INSTRUMENTED_FUNCTIONS) {
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

void safely_copy_argument_zval_as_scalar(zval *original_to_copy, zval *destination)
{
    int len;
    char *ret;

reference_retry_point:
    switch (Z_TYPE_P(original_to_copy)) {
        case IS_NULL:
        case IS_TRUE:
        case IS_FALSE:
        case IS_LONG:
        case IS_DOUBLE:
        case IS_STRING:
            ZVAL_COPY(destination, original_to_copy);
            return;
        case IS_REFERENCE:
            original_to_copy = Z_REFVAL_P(original_to_copy);
            goto reference_retry_point;
        case IS_ARRAY:
            /**
             * @todo improvement for future; copy array but only if it contains scalar
             * @link https://github.com/scoutapp/scout-apm-php-ext/issues/76
             */
            ret = (char*)"(array)";
            break;
        case IS_RESOURCE:
            DYNAMIC_MALLOC_SPRINTF(ret, len,
                "resource(%d)",
                Z_RES_HANDLE_P(original_to_copy)
            );
            break;
        case IS_OBJECT:
            DYNAMIC_MALLOC_SPRINTF(ret, len,
                "object(%s)",
                ZSTR_VAL(Z_OBJ_HT_P(original_to_copy)->get_class_name(Z_OBJ_P(original_to_copy)))
            );
            break;
        default:
            ret = (char*)"(unknown)";
    }

    ZVAL_STRING(destination, ret);
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
