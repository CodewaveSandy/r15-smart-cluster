cmake_minimum_required(VERSION 3.21)
include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(r15_vehicle)
idf_build_set_property(CXX_COMPILE_OPTIONS "-std=gnu++17" APPEND)
