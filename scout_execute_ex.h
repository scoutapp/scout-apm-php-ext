/*
 * Scout APM extension for PHP
 *
 * Copyright (C) 2021-
 * For license information, please see the LICENSE file.
 */

#ifndef SCOUTAPM_SCOUT_EXECUTE_EX_H
#define SCOUTAPM_SCOUT_EXECUTE_EX_H

#define ADD_FUNCTION_TO_INSTRUMENTATION_SAFE_CATCH(function_name)                                             \
    zend_try {                                                                                                \
        add_function_to_instrumentation(function_name);                                                       \
    } zend_catch {                                                                                            \
        php_printf("ScoutAPM tried instrumenting '%s' - increase MAX_INSTRUMENTED_FUNCTIONS", function_name); \
        return FAILURE;                                                                                       \
    } zend_end_try()

#endif //SCOUTAPM_SCOUT_EXECUTE_EX_H
