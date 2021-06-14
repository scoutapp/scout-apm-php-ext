/*
 * Scout APM extension for PHP
 *
 * Copyright (C) 2021
 * For license information, please see the LICENSE file.
 */

#include "zend_scoutapm.h"
#include "scout_extern.h"

#if HAVE_SCOUT_CURL
extern ZEND_NAMED_FUNCTION(scoutapm_curl_setopt_handler);
extern ZEND_NAMED_FUNCTION(scoutapm_curl_exec_handler);
#endif
extern ZEND_NAMED_FUNCTION(scoutapm_fopen_handler);
extern ZEND_NAMED_FUNCTION(scoutapm_fread_handler);
extern ZEND_NAMED_FUNCTION(scoutapm_fwrite_handler);
extern ZEND_NAMED_FUNCTION(scoutapm_pdo_prepare_handler);
extern ZEND_NAMED_FUNCTION(scoutapm_pdostatement_execute_handler);

/* This is simply a map of function names to an index in original_handlers */
indexed_handler_lookup handler_lookup[] = {
    /* define each function we want to overload, which maps to an index in the `original_handlers` array */
    { 0, "file_get_contents"},
    { 1, "file_put_contents"},
    { 2, "curl_setopt"},
    { 3, "curl_exec"},
    { 4, "fopen"},
    { 5, "fread"},
    { 6, "fwrite"},
    { 7, "pdo->exec"},
    { 8, "pdo->query"},
    { 9, "pdo->prepare"},
    {10, "pdostatement->execute"},
    {11, "redis->append"},
    {12, "redis->decr"},
    {13, "redis->decrby"},
    {14, "redis->get"},
    {15, "redis->getbit"},
    {16, "redis->getrange"},
    {17, "redis->getset"},
    {18, "redis->incr"},
    {19, "redis->incrby"},
    {20, "redis->mget"},
    {21, "redis->mset"},
    {22, "redis->msetnx"},
    {23, "redis->set"},
    {24, "redis->setbit"},
    {25, "redis->setex"},
    {26, "redis->psetex"},
    {27, "redis->setnx"},
    {28, "redis->setrange"},
    {29, "redis->strlen"},
    {30, "redis->del"},
    {31, "memcached->get"},
    {32, "memcached->set"},
};
const int handler_lookup_size = sizeof(handler_lookup);

/* handlers count needs to be bigger than the number of handler_lookup entries */
#define ORIGINAL_HANDLERS_TO_ALLOCATE 40
zif_handler original_handlers[ORIGINAL_HANDLERS_TO_ALLOCATE] = {NULL};

int setup_recording_for_internal_handlers()
{
    zend_function *original_function;
    int handler_index;
    zend_class_entry *ce;

    SCOUT_OVERLOAD_FUNCTION("file_get_contents", scoutapm_default_handler)
    SCOUT_OVERLOAD_FUNCTION("file_put_contents", scoutapm_default_handler)
#if HAVE_SCOUT_CURL
    SCOUT_OVERLOAD_FUNCTION("curl_setopt", scoutapm_curl_setopt_handler)
    SCOUT_OVERLOAD_FUNCTION("curl_exec", scoutapm_curl_exec_handler)
#endif
    SCOUT_OVERLOAD_FUNCTION("fopen", scoutapm_fopen_handler)
    SCOUT_OVERLOAD_FUNCTION("fwrite", scoutapm_fwrite_handler)
    SCOUT_OVERLOAD_FUNCTION("fread", scoutapm_fread_handler)
    SCOUT_OVERLOAD_METHOD("pdo", "exec", scoutapm_default_handler)
    SCOUT_OVERLOAD_METHOD("pdo", "query", scoutapm_default_handler)
    SCOUT_OVERLOAD_METHOD("pdo", "prepare", scoutapm_pdo_prepare_handler)
    SCOUT_OVERLOAD_METHOD("pdostatement", "execute", scoutapm_pdostatement_execute_handler)

    SCOUT_OVERLOAD_METHOD("redis", "append", scoutapm_default_handler)
    SCOUT_OVERLOAD_METHOD("redis", "decr", scoutapm_default_handler)
    SCOUT_OVERLOAD_METHOD("redis", "decrby", scoutapm_default_handler)
    SCOUT_OVERLOAD_METHOD("redis", "get", scoutapm_default_handler)
    SCOUT_OVERLOAD_METHOD("redis", "getbit", scoutapm_default_handler)
    SCOUT_OVERLOAD_METHOD("redis", "getrange", scoutapm_default_handler)
    SCOUT_OVERLOAD_METHOD("redis", "getset", scoutapm_default_handler)
    SCOUT_OVERLOAD_METHOD("redis", "incr", scoutapm_default_handler)
    SCOUT_OVERLOAD_METHOD("redis", "incrby", scoutapm_default_handler)
    SCOUT_OVERLOAD_METHOD("redis", "mget", scoutapm_default_handler)
    SCOUT_OVERLOAD_METHOD("redis", "mset", scoutapm_default_handler)
    SCOUT_OVERLOAD_METHOD("redis", "msetnx", scoutapm_default_handler)
    SCOUT_OVERLOAD_METHOD("redis", "set", scoutapm_default_handler)
    SCOUT_OVERLOAD_METHOD("redis", "setbit", scoutapm_default_handler)
    SCOUT_OVERLOAD_METHOD("redis", "setex", scoutapm_default_handler)
    SCOUT_OVERLOAD_METHOD("redis", "psetex", scoutapm_default_handler)
    SCOUT_OVERLOAD_METHOD("redis", "setnx", scoutapm_default_handler)
    SCOUT_OVERLOAD_METHOD("redis", "setrange", scoutapm_default_handler)
    SCOUT_OVERLOAD_METHOD("redis", "strlen", scoutapm_default_handler)
    SCOUT_OVERLOAD_METHOD("redis", "del", scoutapm_default_handler)

    SCOUT_OVERLOAD_METHOD("memcached", "get", scoutapm_default_handler)
    SCOUT_OVERLOAD_METHOD("memcached", "set", scoutapm_default_handler)

    return SUCCESS;
}

/*
 * This handles most recorded calls by grabbing all arguments (we treat it as a variadic), finding the "original" handler
 * for the function and calling it. Once the function is called, record how long it took. Some "special" calls (e.g.
 * curl_exec) have special handling because the arguments to curl_exec don't indicate the URL, for example.
 */
ZEND_NAMED_FUNCTION(scoutapm_default_handler)
{
    int handler_index;
    double entered = scoutapm_microtime();
    int argc;
    zval *argv = NULL;
    const char *called_function;

    SCOUT_PASSTHRU_IF_ALREADY_INSTRUMENTING()

    /* note - no strdup needed as we copy it in record_observed_stack_frame */
    called_function = determine_function_name(execute_data);

    ZEND_PARSE_PARAMETERS_START(0, -1)
            Z_PARAM_VARIADIC(' ', argv, argc)
    ZEND_PARSE_PARAMETERS_END();

    handler_index = handler_index_for_function(called_function);

    original_handlers[handler_index](INTERNAL_FUNCTION_PARAM_PASSTHRU);

    record_observed_stack_frame(called_function, entered, scoutapm_microtime(), argc, argv);
}

int unchecked_handler_index_for_function(const char *function_to_lookup)
{
    int i = 0;
    const char *current = handler_lookup[i].function_name;

    while (current) {
        if (strcasecmp(current, function_to_lookup) == 0) {
            if (handler_lookup[i].index >= ORIGINAL_HANDLERS_TO_ALLOCATE) {
                /*
                 * Note: as this is done in startup, and a critical failure, php_error_docref or zend_throw_exception_ex
                 * aren't suitable here, as they are "exceptions"; therefore the most informative thing we can do is
                 * write a message directly, and return -1, capture this in the PHP_RINIT_FUNCTION and return FAILURE.
                 * The -1 should be checked in SCOUT_OVERLOAD_CLASS_ENTRY_FUNCTION and SCOUT_OVERLOAD_FUNCTION
                 */
                php_printf("ScoutAPM overwrote a handler for '%s' but but we did not allocate enough original_handlers", function_to_lookup);
                return -1;
            }

            return handler_lookup[i].index;
        }
        current = handler_lookup[++i].function_name;
    }

    /* Practically speaking, this shouldn't happen as long as we defined the handlers properly */
    zend_throw_exception_ex(NULL, 0, "ScoutAPM overwrote a handler for '%s' but did not have a handler lookup for it", function_to_lookup);
    return -1;
}

/*
 * In our handler_lookup, find what the "index" in our overridden handlers is for a particular function name
 */
int handler_index_for_function(const char *function_to_lookup)
{
    int handler_index = unchecked_handler_index_for_function(function_to_lookup);

    if (original_handlers[handler_index] == NULL) {
        zend_throw_exception_ex(NULL, 0, "ScoutAPM overwrote a handler for '%s' but the handler for index '%d' was not defined", function_to_lookup, handler_lookup[handler_index].index);
        return -1;
    }

    return handler_index;
}
