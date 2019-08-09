cmake_minimum_required(VERSION 3.9)
project(scoutapm)

set(CMAKE_CXX_STANDARD 11)

include_directories(/home/james/workspace/php-src/main)
include_directories(/home/james/workspace/php-src/Zend)
include_directories(/home/james/workspace/php-src)

add_executable(scoutapm
        zend_scoutapm.c
        zend_scoutapm.h
)
