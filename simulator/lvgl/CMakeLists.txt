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
 "${CLUSTER_ROOT}/embedded/ui/components/Header.cpp"
 "${CLUSTER_ROOT}/embedded/ui/components/SettingsHome.cpp"
 "${CLUSTER_ROOT}/embedded/ui/components/AppearancePanel.cpp"
 "${CLUSTER_ROOT}/embedded/ui/components/BluetoothPanel.cpp"
 "${CLUSTER_ROOT}/embedded/ui/components/WifiPanel.cpp"
 "${CLUSTER_ROOT}/embedded/ui/components/TellTaleRow.cpp"
 "${CLUSTER_ROOT}/embedded/ui/components/Tachometer.cpp"
 "${CLUSTER_ROOT}/embedded/ui/components/PrimaryReadout.cpp"
 "${CLUSTER_ROOT}/embedded/ui/components/RideInfoPanel.cpp"
 "${CLUSTER_ROOT}/embedded/ui/components/FuelGauge.cpp"
 "${CLUSTER_ROOT}/embedded/ui/components/LifecycleOverlay.cpp"
 "${CLUSTER_ROOT}/embedded/ui/Icons.cpp"
 "${CLUSTER_ROOT}/assets/animations/lvgl/r15_signature.c"
 "${CLUSTER_ROOT}/assets/icons/lvgl/cluster_icons_32.c"
 "${CLUSTER_ROOT}/assets/icons/lvgl/cluster_icons_30.c"
 "${CLUSTER_ROOT}/assets/fonts/lvgl/inter_caption_12.c"
 "${CLUSTER_ROOT}/assets/fonts/lvgl/inter_label_14.c"
 "${CLUSTER_ROOT}/assets/fonts/lvgl/inter_value_16.c"
 "${CLUSTER_ROOT}/assets/fonts/lvgl/inter_rpm_16.c"
 "${CLUSTER_ROOT}/assets/fonts/lvgl/inter_unit_18.c"
 "${CLUSTER_ROOT}/assets/fonts/lvgl/inter_heading_23.c"
 "${CLUSTER_ROOT}/assets/fonts/lvgl/racing_preview_28.c"
 "${CLUSTER_ROOT}/assets/fonts/lvgl/racing_secondary_46.c"
 "${CLUSTER_ROOT}/assets/fonts/lvgl/racing_gear_72.c"
 "${CLUSTER_ROOT}/assets/fonts/lvgl/racing_speed_128.c"
 "${CLUSTER_ROOT}/assets/fonts/lvgl/racing_track_74.c"
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
 add_executable(light-signature-check "${CLUSTER_ROOT}/tests/embedded/light-signature-check.cpp")
 target_include_directories(light-signature-check PRIVATE "${CLUSTER_ROOT}")
 target_compile_options(light-signature-check PRIVATE -Wall -Wextra -Wpedantic)
 add_test(NAME light-signature COMMAND light-signature-check)
 add_executable(presentation-input-check "${CLUSTER_ROOT}/tests/embedded/presentation-input-check.cpp")
 target_include_directories(presentation-input-check PRIVATE "${CLUSTER_ROOT}")
 target_compile_options(presentation-input-check PRIVATE -Wall -Wextra -Wpedantic)
 add_test(NAME presentation-input COMMAND presentation-input-check)
 add_executable(desktop-input-check "${CLUSTER_ROOT}/tests/embedded/desktop-input-check.cpp")
 target_include_directories(desktop-input-check PRIVATE "${CLUSTER_ROOT}")
 target_link_libraries(desktop-input-check PRIVATE SDL2::SDL2)
 target_compile_options(desktop-input-check PRIVATE -Wall -Wextra -Wpedantic)
 add_test(NAME desktop-input COMMAND desktop-input-check)
 add_executable(wifi-check "${CLUSTER_ROOT}/tests/embedded/wifi-check.cpp")
 target_include_directories(wifi-check PRIVATE "${CLUSTER_ROOT}")
 target_compile_options(wifi-check PRIVATE -Wall -Wextra -Wpedantic)
 add_test(NAME wifi COMMAND wifi-check)
 add_executable(bluetooth-check "${CLUSTER_ROOT}/tests/embedded/bluetooth-check.cpp")
 target_include_directories(bluetooth-check PRIVATE "${CLUSTER_ROOT}")
 target_compile_options(bluetooth-check PRIVATE -Wall -Wextra -Wpedantic)
 add_test(NAME bluetooth COMMAND bluetooth-check)
 add_executable(preferences-check "${CLUSTER_ROOT}/tests/embedded/preferences-check.cpp")
 target_include_directories(preferences-check PRIVATE "${CLUSTER_ROOT}")
 target_compile_options(preferences-check PRIVATE -Wall -Wextra -Wpedantic)
 add_test(NAME preferences COMMAND preferences-check "${CMAKE_BINARY_DIR}")
 add_executable(typography-check "${CLUSTER_ROOT}/tests/embedded/typography-check.cpp")
 target_link_libraries(typography-check PRIVATE cluster_ui)
 add_test(NAME typography-bounds COMMAND typography-check)
 add_test(NAME lvgl-render COMMAND r15-preview --render "${CMAKE_BINARY_DIR}/previews")
 set_tests_properties(lvgl-render PROPERTIES ENVIRONMENT "SDL_VIDEODRIVER=dummy" TIMEOUT 30)
endif()
