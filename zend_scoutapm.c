#include "zend_scoutapm.h"

static PHP_RINIT_FUNCTION(scoutapm);
static PHP_RSHUTDOWN_FUNCTION(scoutapm);
static int zend_scoutapm_startup(zend_extension*);
static void zend_scoutapm_activate(void);
static void zend_scoutapm_deactivate(void);
static void zend_scoutapm_fcall_begin_handler(zend_execute_data *execute_data);
static void zend_scoutapm_fcall_end_handler(zend_execute_data *execute_data);
static boolean_e is_observed_function(const char *function_name);
PHP_FUNCTION(scoutapm_get_calls);

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
    DEBUG("Initialising stacks...");
    SCOUTAPM_G(stack_depth) = 0;
    SCOUTAPM_G(current_function_stack) = calloc(0, sizeof(scoutapm_stack_frame));

    SCOUTAPM_G(observed_stack_frames_count) = 0;
    SCOUTAPM_G(observed_stack_frames) = calloc(0, sizeof(scoutapm_stack_frame));
    DEBUG("Stacks made\n");
}

static PHP_RSHUTDOWN_FUNCTION(scoutapm)
{
    DEBUG("Freeing stacks... ");
    if (SCOUTAPM_G(current_function_stack)) {
        free(SCOUTAPM_G(current_function_stack));
    }
    SCOUTAPM_G(stack_depth) = 0;

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

static void enter_stack_frame(const char *entered_function_name, double microtime_entered)
{
    DEBUG("Entering stack frame %s ...", entered_function_name);
    SCOUTAPM_G(current_function_stack) = realloc(
        SCOUTAPM_G(current_function_stack),
        (SCOUTAPM_G(stack_depth)+1) * sizeof(scoutapm_stack_frame)
    );
    SCOUTAPM_G(current_function_stack)[SCOUTAPM_G(stack_depth)] = (scoutapm_stack_frame){
        .function_name = entered_function_name,
        .entered = microtime_entered,
    };
    SCOUTAPM_G(stack_depth)++;
    DEBUG("Done\n");
}

static void leave_stack_frame()
{
    DEBUG("Leaving stack frame %s...", SCOUTAPM_CURRENT_STACK_FRAME.function_name);
    SCOUTAPM_G(current_function_stack) = realloc(
        SCOUTAPM_G(current_function_stack),
        (SCOUTAPM_G(stack_depth)-1) * sizeof(scoutapm_stack_frame)
    );
    SCOUTAPM_G(stack_depth)--;
    DEBUG("Done\n");
}

static void zend_scoutapm_fcall_begin_handler(zend_execute_data *execute_data) {
    size_t stack_frame_name_size;
    char *stack_frame_name;

    if (!execute_data->call) {
        zend_op n = execute_data->func->op_array.opcodes[(execute_data->opline - execute_data->func->op_array.opcodes) + 1];
        if (n.extended_value == ZEND_EVAL) {
            stack_frame_name_size = snprintf(NULL, 0, "<evaled code:%s:%u>", ZSTR_VAL(execute_data->func->op_array.filename), n.lineno);
            stack_frame_name = (char*)malloc(stack_frame_name_size);
            sprintf(stack_frame_name, "<evaled code:%s:%u>", ZSTR_VAL(execute_data->func->op_array.filename), n.lineno);
        } else {
            stack_frame_name_size = snprintf(NULL, 0, "<required file>");
            stack_frame_name = (char*)malloc(stack_frame_name_size);
            sprintf(stack_frame_name, "<required file>");
            // @todo add back in
//            zend_string *file = zval_get_string(EX_CONSTANT(n.op1));
//            sprintf(stack_frame_name, "<required file:%s>", ZSTR_VAL(file));
//            zend_string_release(file);
        }
    } else if (execute_data->call->func->common.fn_flags & ZEND_ACC_STATIC) {
        stack_frame_name_size = snprintf(NULL, 0, "%s::%s", ZSTR_VAL(Z_CE(execute_data->call->This)->name), ZSTR_VAL(execute_data->call->func->common.function_name));
        stack_frame_name = (char*)malloc(stack_frame_name_size);
        sprintf(stack_frame_name, "%s::%s", ZSTR_VAL(Z_CE(execute_data->call->This)->name), ZSTR_VAL(execute_data->call->func->common.function_name));
    } else if (Z_TYPE(execute_data->call->This) == IS_OBJECT) {
        stack_frame_name_size = snprintf(NULL, 0, "%s->%s", ZSTR_VAL(Z_OBJCE(execute_data->call->This)->name), ZSTR_VAL(execute_data->call->func->common.function_name));
        stack_frame_name = (char*)malloc(stack_frame_name_size);
        sprintf(stack_frame_name, "%s->%s", ZSTR_VAL(Z_OBJCE(execute_data->call->This)->name), ZSTR_VAL(execute_data->call->func->common.function_name));
    } else if (execute_data->call->func->common.function_name) {
        stack_frame_name_size = snprintf(NULL, 0, "%s", ZSTR_VAL(execute_data->call->func->common.function_name));
        stack_frame_name = (char*)malloc(stack_frame_name_size);
        sprintf(stack_frame_name, "%s", ZSTR_VAL(execute_data->call->func->common.function_name));
    } else {
        DEBUG("POSSIBLE BUG : no stack frame name detected...\n");
        stack_frame_name_size = snprintf(NULL, 0, "<unknown name>");
        stack_frame_name = (char*)malloc(stack_frame_name_size);
        sprintf(stack_frame_name, "<unknown name>");
    }

    enter_stack_frame(stack_frame_name, scoutapm_microtime());
}

static void zend_scoutapm_fcall_end_handler(zend_execute_data *execute_data)
{
    DEBUG("handling fcall_end...\n");
    if (SCOUTAPM_G(stack_depth) == 0) {
        DEBUG("POSSIBLE BUG: fcall_end called but nothing was in stack\n");
        return;
    }
    if (is_observed_function(SCOUTAPM_CURRENT_STACK_FRAME.function_name)) {
        const double exit_time = scoutapm_microtime();
        record_observed_stack_frame(SCOUTAPM_CURRENT_STACK_FRAME.function_name, SCOUTAPM_CURRENT_STACK_FRAME.entered, exit_time);
    }

    leave_stack_frame();
}

PHP_FUNCTION(scoutapm_get_calls)
{
    const char *item_key_function = "function";
    const char *item_key_entered = "entered";
    const char *item_key_exited = "exited";
    const char *item_key_time_taken = "time_taken";
    zval item;
    ZEND_PARSE_PARAMETERS_NONE();

//    print_stack_frame(SCOUTAPM_G(observed_stack_frames), SCOUTAPM_G(observed_stack_frames_count));

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
