#include "zend_scoutapm.h"

static PHP_RINIT_FUNCTION(scoutapm);
static PHP_RSHUTDOWN_FUNCTION(scoutapm);
static double scoutapm_microtime();
static void record_observed_stack_frame(const char *function_name, double microtime_entered, double microtime_exited, int argc, zval *argv);
PHP_FUNCTION(scoutapm_get_calls);

SCOUT_DEFINE_OVERLOADED_FUNCTION(file_get_contents);

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

ZEND_GET_MODULE(scoutapm);

// @todo look into making just one function overloader
SCOUT_OVERLOADED_FUNCTION(file_get_contents)

static PHP_RINIT_FUNCTION(scoutapm)
{
    DEBUG("Initialising stacks...");
    SCOUTAPM_G(observed_stack_frames_count) = 0;
    SCOUTAPM_G(observed_stack_frames) = calloc(0, sizeof(scoutapm_stack_frame));
    DEBUG("Stacks made\n");

    if (SCOUTAPM_G(handlers_set) != 1) {
        DEBUG("Overriding function handlers.\n");

        // @todo this could be configurable by INI if more dynamic
        SCOUT_OVERLOAD_FUNCTION(file_get_contents)

        SCOUTAPM_G(handlers_set) = 1;
    } else {
        DEBUG("Handlers have already been set, skipping.\n");
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

static void record_observed_stack_frame(const char *function_name, double microtime_entered, double microtime_exited, int argc, zval *argv)
{
    if (argc > 0) {
        DEBUG("Adding observed stack frame for %s (%s) ... ", function_name, Z_STRVAL(argv[0]));
    } else {
        DEBUG("Adding observed stack frame for %s ... ", function_name);
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

    for (int i = 0; i < argc; i++) {
        ZVAL_COPY(
            &(SCOUTAPM_G(observed_stack_frames)[SCOUTAPM_G(observed_stack_frames_count)].argv[i]),
            &(argv[i])
        );
    }

    SCOUTAPM_G(observed_stack_frames_count)++;
    DEBUG("Done\n");
}

PHP_FUNCTION(scoutapm_get_calls)
{
    zval item, arg_items, arg_item;
    ZEND_PARSE_PARAMETERS_NONE();

    DEBUG("scoutapm_get_calls: preparing return value... ");

    array_init(return_value);

    for (int i = 0; i < SCOUTAPM_G(observed_stack_frames_count); i++) {
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

        // Time taken is calculated here because float precision gets knocked off otherwise - so this is a useful metric
        add_assoc_double_ex(
            &item,
            SCOUT_GET_CALLS_KEY_TIME_TAKEN, strlen(SCOUT_GET_CALLS_KEY_TIME_TAKEN),
            SCOUTAPM_G(observed_stack_frames)[i].exited - SCOUTAPM_G(observed_stack_frames)[i].entered
        );

        array_init(&arg_items);
        for (int j = 0; j < SCOUTAPM_G(observed_stack_frames)[i].argc; j++) {
            // Must copy the argument to a new zval, otherwise it gets freed and we get segfault.
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

    DEBUG("done.\n");
}
