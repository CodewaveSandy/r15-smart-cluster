cmake_minimum_required(VERSION 3.21)
include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(r15_display)
idf_build_set_property(CXX_COMPILE_OPTIONS "-std=gnu++17" APPEND)
# Apply the same focused feature profile to LVGL and its consumers.
idf_build_set_property(COMPILE_DEFINITIONS "LV_CONF_PATH=${CMAKE_CURRENT_LIST_DIR}/include/lv_conf.h" APPEND)
idf_build_set_property(COMPILE_DEFINITIONS "LV_KCONFIG_IGNORE=1" APPEND)
idf_build_set_property(COMPILE_DEFINITIONS "LV_LVGL_H_INCLUDE_SIMPLE=1" APPEND)
