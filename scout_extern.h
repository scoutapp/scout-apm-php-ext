/*
 * Scout APM extension for PHP
 *
 * Copyright (C) 2019-
 * For license information, please see the LICENSE file.
 */

#ifndef SCOUT_API_H
#define SCOUT_API_H

extern ZEND_NAMED_FUNCTION(scoutapm_default_handler);
extern double scoutapm_microtime();
extern void record_arguments_for_call(const char *call_reference, int argc, zval *argv);
extern zend_long find_index_for_recorded_arguments(const char *call_reference);
extern void record_observed_stack_frame(const char *function_name, double microtime_entered, double microtime_exited, int argc, zval *argv);
extern int handler_index_for_function(const char *function_to_lookup);
extern const char* determine_function_name(zend_execute_data *execute_data);
extern const char *unique_resource_id(const char *scout_wrapper_type, zval *resource_id);
extern const char *unique_class_instance_id(zval *class_instance);
extern void safely_copy_argument_zval_as_scalar(zval *original_to_copy, zval *destination);
extern int unchecked_handler_index_for_function(const char *function_to_lookup);
extern int should_be_instrumented(const char *function_name);

ZEND_EXTERN_MODULE_GLOBALS(scoutapm);
extern indexed_handler_lookup handler_lookup[];
extern const int handler_lookup_size;
extern zif_handler original_handlers[];

#endif /* SCOUT_API_H */
