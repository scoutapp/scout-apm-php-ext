//
// Created by james on 09/08/2019.
//

#include "zend_scoutapm.h"

static int  zend_scoutapm_startup(zend_extension*);

zend_extension_version_info extension_version_info = {
    ZEND_EXTENSION_API_NO,
    ZEND_EXTENSION_BUILD_ID
};

zend_extension zend_extension_entry = {
    "scoutapm",
    "0.0",
    "Scout APM",
    "https://scoutapm.com/",
    "Copyright Scout APM",
    zend_scoutapm_startup,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    STANDARD_ZEND_EXTENSION_PROPERTIES
};

static int zend_scoutapm_startup(zend_extension *ze) {
    return SUCCESS;
}
