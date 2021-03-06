cmake_minimum_required(VERSION 3.9)
project(scoutapm C)

set(SOURCE_FILES
        zend_scoutapm.c
        scout_curl_wrapper.c
        scout_file_wrapper.c
        scout_pdo_wrapper.c
        scout_extern.h
        zend_scoutapm.h
        scout_utils.c
        scout_functions.c
        scout_execute_ex.c
        scout_observer.c
        scout_internal_handlers.c
        scout_internal_handlers.h
        scout_recording.c
        scout_recording.h
        scout_execute_ex.h
)

execute_process(
        COMMAND php-config --include-dir
        OUTPUT_VARIABLE PHP_SOURCE
)
string(REGEX REPLACE "\n$" "" PHP_SOURCE "${PHP_SOURCE}")
message("Using source directory: ${PHP_SOURCE}")

include_directories(${PHP_SOURCE})
include_directories(${PHP_SOURCE}/main)
include_directories(${PHP_SOURCE}/Zend)
include_directories(${PHP_SOURCE}/TSRM)
include_directories(${PROJECT_SOURCE_DIR})

add_custom_target(configure
        COMMAND phpize && ./configure --enable-scoutapm-dev
        DEPENDS ${SOURCE_FILES}
        WORKING_DIRECTORY ${PROJECT_SOURCE_DIR})

add_library(___ EXCLUDE_FROM_ALL ${SOURCE_FILES})
