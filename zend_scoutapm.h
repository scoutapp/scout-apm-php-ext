//
// Created by james on 09/08/2019.
//

#ifndef ZEND_SCOUTAPM_H
#define ZEND_SCOUTAPM_H

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "php.h"
#include <zend_extensions.h>
#include <zend_compile.h>
#include <zend_exceptions.h>

#define SCOUT_APM_EXT_NAME "scoutapm"
#define SCOUT_APM_EXT_VERSION "0.0"

// Extreme amounts of debugging, set to 1 to enable it and `make clean && make` (tests will fail...)
#define SCOUT_APM_EXT_DEBUGGING 0

typedef struct scoutapm_stack_frame {
    const char *function_name;
    double entered;
    double exited;
} scoutapm_stack_frame;

ZEND_BEGIN_MODULE_GLOBALS(scoutapm)
    zend_long stack_depth;
    scoutapm_stack_frame *current_function_stack;
    zend_long observed_stack_frames_count;
    scoutapm_stack_frame *observed_stack_frames;
ZEND_END_MODULE_GLOBALS(scoutapm)

#ifndef ZEND_PARSE_PARAMETERS_NONE
#define ZEND_PARSE_PARAMETERS_NONE() if (zend_parse_parameters_none() != SUCCESS) { return; }
#endif

#ifdef ZTS
#define SCOUTAPM_G(v) ZEND_MODULE_GLOBALS_ACCESSOR(scoutapm, v)
#else
#define SCOUTAPM_G(v) (scoutapm_globals.v)
#endif

#if SCOUT_APM_EXT_DEBUGGING == 1
#define DEBUG(x, ...) php_printf(x, ##__VA_ARGS__)
#else
#define DEBUG(...) /**/
#endif

#define SCOUTAPM_CURRENT_STACK_FRAME SCOUTAPM_G(current_function_stack)[SCOUTAPM_G(stack_depth)-1]

#endif //ZEND_SCOUTAPM_H
