/*
 * Scout APM extension for PHP
 *
 * Copyright (C) 2021-
 * For license information, please see the LICENSE file.
 */

#ifndef SCOUT_INTERNAL_HANDLERS_H
#define SCOUT_INTERNAL_HANDLERS

typedef struct _handler_lookup {
    int index;
    const char *function_name;
} indexed_handler_lookup;

/* overload a regular function by wrapping its handler with our own handler */
#define SCOUT_OVERLOAD_FUNCTION(function_name, handler_to_use) \
    original_function = zend_hash_str_find_ptr(EG(function_table), function_name, sizeof(function_name) - 1); \
    if (original_function != NULL) { \
        handler_index = unchecked_handler_index_for_function(function_name); \
        if (handler_index < 0) return FAILURE; \
        original_handlers[handler_index] = original_function->internal_function.handler; \
        original_function->internal_function.handler = handler_to_use; \
    }

/* Don't use this macro directly, use SCOUT_OVERLOAD_STATIC_METHOD or SCOUT_OVERLOAD_METHOD for consistency */
#define SCOUT_OVERLOAD_CLASS_ENTRY_FUNCTION(lowercase_class_name, instance_or_static, method_name, handler_to_use) \
    ce = zend_hash_str_find_ptr(CG(class_table), lowercase_class_name, sizeof(lowercase_class_name) - 1); \
    if (ce != NULL) { \
        original_function = zend_hash_str_find_ptr(&ce->function_table, method_name, sizeof(method_name)-1); \
        if (original_function != NULL) { \
            handler_index = unchecked_handler_index_for_function(lowercase_class_name instance_or_static method_name); \
            if (handler_index < 0) return FAILURE; \
            original_handlers[handler_index] = original_function->internal_function.handler; \
            original_function->internal_function.handler = handler_to_use; \
        } \
    }

/* overload a static class method by wrapping its handler with our own handler */
#define SCOUT_OVERLOAD_STATIC_METHOD(lowercase_class_name, method_name, handler_to_use) SCOUT_OVERLOAD_CLASS_ENTRY_FUNCTION(lowercase_class_name, "::", method_name, handler_to_use)

/* overload an instance class method by wrapping its handler with our own handler */
#define SCOUT_OVERLOAD_METHOD(lowercase_class_name, method_name, handler_to_use) SCOUT_OVERLOAD_CLASS_ENTRY_FUNCTION(lowercase_class_name, "->", method_name, handler_to_use)

#define SCOUT_INTERNAL_FUNCTION_PASSTHRU() original_handlers[handler_index_for_function(determine_function_name(execute_data))](INTERNAL_FUNCTION_PARAM_PASSTHRU)

#define SCOUT_PASSTHRU_IF_ALREADY_INSTRUMENTING()   \
    if (SCOUTAPM_G(currently_instrumenting) == 1) { \
        SCOUT_INTERNAL_FUNCTION_PASSTHRU();         \
        return;                                     \
    }

#endif /* SCOUT_INTERNAL_HANDLERS_H */
