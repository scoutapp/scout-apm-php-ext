//
// Created by james on 09/08/2019.
//

#include <zend_compile.h>
#include "zend_scoutapm.h"

static int zend_scoutapm_startup(zend_extension*);
static void zend_scoutapm_activate(void);
static void zend_scoutapm_deactivate(void);
static void zend_scoutapm_fcall_begin_handler(zend_execute_data *execute_data);
static void zend_scoutapm_fcall_end_handler(zend_execute_data *execute_data);
boolean_e is_observed_function(char *function_name);

zend_extension_version_info extension_version_info = {
    ZEND_EXTENSION_API_NO,
    ZEND_EXTENSION_BUILD_ID
};

zend_extension zend_extension_entry = {
    "scoutapm",
    "0.0",
    "Scout APM",
    "https://scoutapm.com/",
    "Copyright Scout APM",
    zend_scoutapm_startup, // extension startup
    NULL, // extension shutdown
    zend_scoutapm_activate, // request startup
    zend_scoutapm_deactivate, // request shutdown
    NULL, // message handler
    NULL, // compiler op_array_ahndler
    NULL, // VM statement_handler
    zend_scoutapm_fcall_begin_handler, // VM fcall_begin_handler
    zend_scoutapm_fcall_end_handler, // VM_fcall_end_handler
    NULL, // compiler op_array_ctor
    NULL, // compiler op_array_dtor
    STANDARD_ZEND_EXTENSION_PROPERTIES
};

static int zend_scoutapm_startup(zend_extension *ze) {
    return SUCCESS;
}

static void zend_scoutapm_activate(void) {
    CG(compiler_options) |= ZEND_COMPILE_EXTENDED_INFO;
}

static void zend_scoutapm_deactivate(void) {
    CG(compiler_options) |= ZEND_COMPILE_EXTENDED_INFO;
}

static void zend_scoutapm_fcall_begin_handler(zend_execute_data *execute_data) {
    if (!execute_data->call) {
        return;
    }

    // @todo take care of namespacing

    if (is_observed_function(ZSTR_VAL(execute_data->call->func->common.function_name))) {
        php_printf("Entered: %s\n", ZSTR_VAL(execute_data->call->func->common.function_name));
    }

    // @todo regardless of whether we are in an interesting function, add something to stack
}

static void zend_scoutapm_fcall_end_handler(zend_execute_data *aexecute_data) {

    // @todo keep track of stack somewhere so we know what we're exiting from, execute_data doesn't seem to have it?
//    php_printf("Exited ");

    // @todo take care of namespacing

//    if (is_observed_function(ZSTR_VAL(execute_data->func->common.function_name))) {
//        php_printf("Exited: %s\n", ZSTR_VAL(execute_data->call->func->common.function_name));
//    }
}

boolean_e is_observed_function(char *function_name)
{
    int i;
    const char *observe_functions[1] = {
        "file_get_contents"
    };

    for (i = 0; i < 1; i++) {
        if (strcmp(function_name, observe_functions[i]) == 0) {
            return YES;
        }
    }

    return NO;
}
