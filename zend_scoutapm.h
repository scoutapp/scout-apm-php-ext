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

#define SCOUT_APM_EXT_NAME "scoutapm"
#define SCOUT_APM_EXT_VERSION "0.0"

ZEND_BEGIN_MODULE_GLOBALS(scoutapm)
    zend_long stack_depth;
ZEND_END_MODULE_GLOBALS(scoutapm)

#ifdef ZTS
#define SCOUTAPM_G(v) ZEND_MODULE_GLOBALS_ACCESSOR(scoutapm, v)
#else
#define SCOUTAPM_G(v) (scoutapm_globals.v)
#endif

#endif //ZEND_SCOUTAPM_H
