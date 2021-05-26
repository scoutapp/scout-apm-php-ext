/*
 * Scout APM extension for PHP
 *
 * Copyright (C) 2021
 * For license information, please see the LICENSE file.
 */

#include "zend_scoutapm.h"
#include "scout_extern.h"

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

/**
 * This function wraps PHP's implementation of str_replace so we don't have to re-implement the same mechanism :)
 */
const char *scout_str_replace(const char *search, const char *replace, const char *subject)
{
    zval args[3];
    zval retval, func;
    const char *replaced_string;

    ZVAL_STRING(&func, "str_replace");

    ZVAL_STRING(&args[0], search);
    ZVAL_STRING(&args[1], replace);
    ZVAL_STRING(&args[2], subject);

    call_user_function(EG(function_table), NULL, &func, &retval, 3, args);

    // Only return strings - if something went wrong, return the original subject
    if (Z_TYPE(retval) != IS_STRING) {
        return subject;
    }

    replaced_string = strdup(Z_STRVAL(retval));

    zval_ptr_dtor(&args[0]);
    zval_ptr_dtor(&args[1]);
    zval_ptr_dtor(&args[2]);
    zval_ptr_dtor(&func);
    zval_ptr_dtor(&retval);

    return replaced_string;
}
