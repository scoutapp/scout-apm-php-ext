#include "zend_scoutapm.h"

static PHP_RINIT_FUNCTION(scoutapm);
static PHP_RSHUTDOWN_FUNCTION(scoutapm);
static double scoutapm_microtime();
static void record_observed_stack_frame(const char *function_name, double microtime_entered, double microtime_exited);
PHP_FUNCTION(scoutapm_get_calls);

zif_handler original_handler_file_get_contents;

ZEND_DECLARE_MODULE_GLOBALS(scoutapm)

static const zend_function_entry scoutapm_functions[] = {
    PHP_FE(scoutapm_get_calls, NULL)
    PHP_FE_END
};

static zend_module_entry scoutapm_module_entry = {
    STANDARD_MODULE_HEADER,
    SCOUT_APM_EXT_NAME,
    scoutapm_functions, // function entries
    NULL, // module init
    NULL, // module shutdown
    PHP_RINIT(scoutapm), // request init
    PHP_RSHUTDOWN(scoutapm), // request shutdown
    NULL, // module information
    SCOUT_APM_EXT_VERSION,
    PHP_MODULE_GLOBALS(scoutapm),
    NULL,
    NULL,
    NULL,
    STANDARD_MODULE_PROPERTIES_EX
};

/*
 * Do not export this module, so it cannot be registered with `extension=scoutapm.so` - must be `zend_extension=`
 * Instead, see `zend_scoutapm_startup` - we load the module there.*/
ZEND_GET_MODULE(scoutapm);
// */

ZEND_NAMED_FUNCTION(scoutapm_file_get_contents)
{
    double entered = scoutapm_microtime();

    int argc;
    zval *args = NULL;

    ZEND_PARSE_PARAMETERS_START(0, -1)
        Z_PARAM_VARIADIC('+', args, argc)
    ZEND_PARSE_PARAMETERS_END();

    original_handler_file_get_contents(INTERNAL_FUNCTION_PARAM_PASSTHRU);

    record_observed_stack_frame("file_get_contents", entered, scoutapm_microtime());
}

static PHP_RINIT_FUNCTION(scoutapm)
{
    DEBUG("Initialising stacks...");
    SCOUTAPM_G(observed_stack_frames_count) = 0;
    SCOUTAPM_G(observed_stack_frames) = calloc(0, sizeof(scoutapm_stack_frame));
    DEBUG("Stacks made\n");

    if (SCOUTAPM_G(handlers_set) != YES) {
        DEBUG("Overriding function handlers.\n");
        zend_function *original_function_file_get_contents;

        original_function_file_get_contents = zend_hash_str_find_ptr(EG(function_table), "file_get_contents", sizeof("file_get_contents")-1);
        if (original_function_file_get_contents != NULL) {
            original_handler_file_get_contents = original_function_file_get_contents->internal_function.handler;
            original_function_file_get_contents->internal_function.handler = scoutapm_file_get_contents;
        }
        SCOUTAPM_G(handlers_set) = YES;
    } else {
        php_printf("Handlers have already been set, skipping.\n");
    }
}

static PHP_RSHUTDOWN_FUNCTION(scoutapm)
{
    DEBUG("Freeing stacks... ");
    if (SCOUTAPM_G(observed_stack_frames)) {
        free(SCOUTAPM_G(observed_stack_frames));
    }
    SCOUTAPM_G(observed_stack_frames_count) = 0;
    DEBUG("Stacks freed\n");
}

// Note - useful for debugging, can probably be removed
static void print_stack_frame(scoutapm_stack_frame *stack_frame, zend_long depth)
{
    if (SCOUT_APM_EXT_DEBUGGING != 1) {
        return;
    }

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
    DEBUG("Adding observed stack frame for %s ... ", function_name);
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
    DEBUG("Done\n");
}

PHP_FUNCTION(scoutapm_get_calls)
{
    const char *item_key_function = "function";
    const char *item_key_entered = "entered";
    const char *item_key_exited = "exited";
    const char *item_key_time_taken = "time_taken";
    zval item;
    ZEND_PARSE_PARAMETERS_NONE();

    array_init(return_value);

    for (int i = 0; i < SCOUTAPM_G(observed_stack_frames_count); i++) {
        array_init(&item);

        add_assoc_str_ex(
            &item,
            item_key_function, strlen(item_key_function),
            zend_string_init(SCOUTAPM_G(observed_stack_frames)[i].function_name, strlen(SCOUTAPM_G(observed_stack_frames)[i].function_name), 0)
        );

        add_assoc_double_ex(
            &item,
            item_key_entered, strlen(item_key_entered),
            SCOUTAPM_G(observed_stack_frames)[i].entered
        );

        add_assoc_double_ex(
            &item,
            item_key_exited, strlen(item_key_exited),
            SCOUTAPM_G(observed_stack_frames)[i].exited
        );

        // Time taken is calculated here because float precision gets knocked off otherwise - so this is a useful metric
        add_assoc_double_ex(
            &item,
            item_key_time_taken, strlen(item_key_time_taken),
            SCOUTAPM_G(observed_stack_frames)[i].exited - SCOUTAPM_G(observed_stack_frames)[i].entered
        );

        add_next_index_zval(return_value, &item);
    }

    SCOUTAPM_G(observed_stack_frames) = realloc(SCOUTAPM_G(observed_stack_frames), 0);
    SCOUTAPM_G(observed_stack_frames_count) = 0;
}
