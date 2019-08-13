#include "zend_scoutapm.h"

static PHP_RINIT_FUNCTION(scoutapm);
static int zend_scoutapm_startup(zend_extension*);
static void zend_scoutapm_activate(void);
static void zend_scoutapm_deactivate(void);
static void zend_scoutapm_fcall_begin_handler(zend_execute_data *execute_data);
static void zend_scoutapm_fcall_end_handler(zend_execute_data *execute_data);
static boolean_e is_observed_function(const char *function_name);

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
    SCOUTAPM_G(observed_stack_frames_count) = 0;
    SCOUTAPM_G(current_function_stack) = calloc(0, sizeof(scoutapm_stack_frame));
    SCOUTAPM_G(observed_stack_frames) = calloc(0, sizeof(scoutapm_stack_frame));
}

// Note - useful for debugging, can probably be removed
static void print_stack_frame(scoutapm_stack_frame *stack_frame, zend_long depth)
{
    php_printf("  Stack Record:\n");
    for (int i = 0; i < depth; i++) {
        php_printf(
            "    %d:%s\n      + %f\n      - %f\n",
            i,
            stack_frame[i].function_name,
            stack_frame[i].entered,
            stack_frame[i].exited
        );
    }
    php_printf("\n");
}

// @todo we could just use already implemented microtime(true) ?
static double scoutapm_microtime()
{
    struct timeval tp = {0};
    if (gettimeofday(&tp, NULL)) {
        zend_throw_exception_ex(zend_ce_exception, 0, "Could not call gettimeofday");
        return 0;
    }
    return (double)(tp.tv_sec + tp.tv_usec / 1000000.00);
}

static void record_observed_stack_frame(const char *function_name, double microtime_entered, double microtime_exited)
{
    SCOUTAPM_G(observed_stack_frames) = realloc(
        SCOUTAPM_G(observed_stack_frames),
        (SCOUTAPM_G(observed_stack_frames_count)+1) * sizeof(scoutapm_stack_frame)
    );
    SCOUTAPM_G(observed_stack_frames)[SCOUTAPM_G(observed_stack_frames_count)] = (scoutapm_stack_frame){
        .function_name = function_name,
        .entered = microtime_entered,
        .exited = microtime_exited
    };
    SCOUTAPM_G(observed_stack_frames_count)++;

//    print_stack_frame(SCOUTAPM_G(observed_stack_frames), SCOUTAPM_G(observed_stack_frames_count));
}

static void enter_stack_frame(const char *entered_function_name, double microtime_entered)
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

    enter_stack_frame(ZSTR_VAL(execute_data->call->func->common.function_name), scoutapm_microtime());

    // @todo possibly need to free the stack in RSHUTDOWN ..? I suppose only needed if there's anything left...

    if (is_observed_function(SCOUTAPM_CURRENT_STACK_FRAME.function_name)) {
//        php_printf("Entered @ %f: %s\n", SCOUTAPM_CURRENT_STACK_FRAME.entered, SCOUTAPM_CURRENT_STACK_FRAME.function_name);
    }
}

static void zend_scoutapm_fcall_end_handler(zend_execute_data *execute_data)
{
    // @todo take care of namespacing - https://github.com/scoutapp/scout-apm-php-ext/issues/2
    if (is_observed_function(SCOUTAPM_CURRENT_STACK_FRAME.function_name)) {
        const double exit_time = scoutapm_microtime();
        record_observed_stack_frame(SCOUTAPM_CURRENT_STACK_FRAME.function_name, SCOUTAPM_CURRENT_STACK_FRAME.entered, exit_time);
//        php_printf("Exited  @ %f: %s\n", exit_time, SCOUTAPM_CURRENT_STACK_FRAME.function_name);
    }

    leave_stack_frame();
}

static boolean_e is_observed_function(const char *function_name)
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
