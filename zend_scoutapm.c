#include "zend_scoutapm.h"

static PHP_RINIT_FUNCTION(scoutapm);
static int zend_scoutapm_startup(zend_extension*);
static void zend_scoutapm_activate(void);
static void zend_scoutapm_deactivate(void);
static void zend_scoutapm_fcall_begin_handler(zend_execute_data *execute_data);
static void zend_scoutapm_fcall_end_handler(zend_execute_data *execute_data);
static boolean_e is_observed_function(char *function_name);

ZEND_DECLARE_MODULE_GLOBALS(scoutapm)

static zend_module_entry scoutapm_module_entry = {
    STANDARD_MODULE_HEADER,
    SCOUT_APM_EXT_NAME,
    NULL, // function entries
    NULL, // module init
    NULL, // module shutdown
    PHP_RINIT(scoutapm), // request init
    NULL, // request shutdown
    NULL, // module information
    SCOUT_APM_EXT_VERSION,
    STANDARD_MODULE_PROPERTIES
};

/*
 * Do not export this module, so it cannot be registered with `extension=scoutapm.so` - must be `zend_extension=`
 * Instead, see `zend_scoutapm_startup` - we load the module there.
ZEND_GET_MODULE(scoutapm);
 */

zend_extension_version_info extension_version_info = {
    ZEND_EXTENSION_API_NO,
    ZEND_EXTENSION_BUILD_ID
};

zend_extension zend_extension_entry = {
    SCOUT_APM_EXT_NAME,
    SCOUT_APM_EXT_VERSION,
    "Scout APM",
    "https://scoutapm.com/",
    "Copyright 2019",
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
    return zend_startup_module(&scoutapm_module_entry);
}

static void zend_scoutapm_activate(void) {
    CG(compiler_options) |= ZEND_COMPILE_EXTENDED_INFO;
}

static void zend_scoutapm_deactivate(void) {
    CG(compiler_options) |= ZEND_COMPILE_EXTENDED_INFO;
}

static PHP_RINIT_FUNCTION(scoutapm)
{
    SCOUTAPM_G(stack_depth) = 0;
    SCOUTAPM_G(current_function_stack) = calloc(0, sizeof(scoutapm_stack_frame));
}

static void print_stack_frame()
{
    php_printf("       CURRENT STACK: ");
    for (int i = 0; i < SCOUTAPM_G(stack_depth); i++) {
        php_printf("    %d:%s ", i, SCOUTAPM_G(current_function_stack)[i].function_name);
    }
    php_printf("\n");
}

static void enter_stack_frame(char *entered_function_name, float microtime_entered)
{
    SCOUTAPM_G(current_function_stack) = realloc(
        SCOUTAPM_G(current_function_stack),
        (SCOUTAPM_G(stack_depth)+1) * sizeof(scoutapm_stack_frame)
    );
    SCOUTAPM_G(current_function_stack)[SCOUTAPM_G(stack_depth)] = (scoutapm_stack_frame){
        .function_name = entered_function_name,
        .entered = microtime_entered,
    };
    SCOUTAPM_G(stack_depth)++;
}

static void leave_stack_frame()
{
    SCOUTAPM_G(current_function_stack) = realloc(
        SCOUTAPM_G(current_function_stack),
        (SCOUTAPM_G(stack_depth)-1) * sizeof(scoutapm_stack_frame)
    );
    SCOUTAPM_G(stack_depth)--;
}

static void zend_scoutapm_fcall_begin_handler(zend_execute_data *execute_data) {
    if (!execute_data->call) {
        return;
    }

    // @todo take care of namespacing - https://github.com/scoutapp/scout-apm-php-ext/issues/2

    // @todo microtime(true)
    enter_stack_frame(ZSTR_VAL(execute_data->call->func->common.function_name), 0);

    // @todo possibly need to free the stack in RSHUTDOWN ..? I suppose only needed if there's anything left...

    if (is_observed_function(SCOUTAPM_CURRENT_STACK_FRAME.function_name)) {
        php_printf("Entered: %s\n", SCOUTAPM_CURRENT_STACK_FRAME.function_name);
    }
}

static void zend_scoutapm_fcall_end_handler(zend_execute_data *execute_data)
{
    scoutapm_stack_frame exiting_stack_frame = {
        .function_name = SCOUTAPM_CURRENT_STACK_FRAME.function_name,
        .entered = SCOUTAPM_CURRENT_STACK_FRAME.entered
    };

    leave_stack_frame();

    if (is_observed_function(exiting_stack_frame.function_name)) {
        php_printf("Exited: %s\n", exiting_stack_frame.function_name);
    }

    // @todo take care of namespacing - https://github.com/scoutapp/scout-apm-php-ext/issues/2
}

static boolean_e is_observed_function(char *function_name)
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
