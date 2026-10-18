cmake_minimum_required(VERSION 3.21)
project(R15Preview VERSION 0.1.0 LANGUAGES C CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
get_filename_component(CLUSTER_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../.." ABSOLUTE)
include(FetchContent)
set(LV_CONF_PATH "${CMAKE_CURRENT_SOURCE_DIR}/lv_conf.h" CACHE STRING "" FORCE)
set(LV_CONF_BUILD_DISABLE_EXAMPLES ON CACHE BOOL "" FORCE)
set(LV_CONF_BUILD_DISABLE_DEMOS ON CACHE BOOL "" FORCE)
set(LV_CONF_BUILD_DISABLE_THORVG_INTERNAL ON CACHE BOOL "" FORCE)
FetchContent_Declare(lvgl
 URL https://codeload.github.com/lvgl/lvgl/tar.gz/refs/tags/v9.2.2
 URL_HASH SHA256=129b4e00e06639fa79d7e8a6cab3c1ecce2445b1a246652ccd34f22e7b17ad6f
 DOWNLOAD_EXTRACT_TIMESTAMP TRUE)
FetchContent_MakeAvailable(lvgl)
find_package(SDL2 REQUIRED CONFIG)
add_library(cluster_ui STATIC
 "${CLUSTER_ROOT}/embedded/ui/Dashboard.cpp"
 "${CLUSTER_ROOT}/assets/fonts/lvgl/inter_caption_12.c"
 "${CLUSTER_ROOT}/assets/fonts/lvgl/inter_label_14.c"
 "${CLUSTER_ROOT}/assets/fonts/lvgl/inter_value_16.c"
 "${CLUSTER_ROOT}/assets/fonts/lvgl/inter_unit_18.c"
 "${CLUSTER_ROOT}/assets/fonts/lvgl/inter_heading_23.c"
 "${CLUSTER_ROOT}/assets/fonts/lvgl/barlow_gear_46.c"
 "${CLUSTER_ROOT}/assets/fonts/lvgl/barlow_speed_128.c"
 "${CLUSTER_ROOT}/assets/fonts/lvgl/barlow_track_79.c"
 "${CLUSTER_ROOT}/assets/fonts/lvgl/barlow_clock_30.c"
)
target_include_directories(cluster_ui PUBLIC "${CLUSTER_ROOT}")
target_link_libraries(cluster_ui PUBLIC lvgl::lvgl)
target_compile_options(cluster_ui PRIVATE -Wall -Wextra -Wpedantic)
add_executable(r15-preview MACOSX_BUNDLE main.cpp)
set_target_properties(r15-preview PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}"
 MACOSX_BUNDLE_BUNDLE_NAME "R15 Preview"
 MACOSX_BUNDLE_GUI_IDENTIFIER "dev.r15smartcluster.preview")
target_link_libraries(r15-preview PRIVATE cluster_ui SDL2::SDL2)
target_compile_options(r15-preview PRIVATE -Wall -Wextra -Wpedantic)
include(CTest)
if(BUILD_TESTING)
 add_executable(model-check "${CLUSTER_ROOT}/tests/embedded/model-check.cpp")
 target_include_directories(model-check PRIVATE "${CLUSTER_ROOT}")
 add_test(NAME model-lifecycle COMMAND model-check)
 add_executable(typography-check "${CLUSTER_ROOT}/tests/embedded/typography-check.cpp")
 target_link_libraries(typography-check PRIVATE cluster_ui)
 add_test(NAME typography-bounds COMMAND typography-check)
 add_test(NAME lvgl-render COMMAND r15-preview --render "${CMAKE_BINARY_DIR}/previews")
 set_tests_properties(lvgl-render PROPERTIES ENVIRONMENT "SDL_VIDEODRIVER=dummy" TIMEOUT 30)
endif()
