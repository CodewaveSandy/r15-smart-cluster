get_filename_component(CLUSTER_ROOT "${CMAKE_CURRENT_LIST_DIR}/../../.." ABSOLUTE)
file(GLOB CLUSTER_COMPONENTS "${CLUSTER_ROOT}/embedded/ui/components/*.cpp")
file(GLOB CLUSTER_FONTS "${CLUSTER_ROOT}/assets/fonts/lvgl/*.c")
idf_component_register(
 SRCS main.cpp CrowPanel.cpp
 "${CLUSTER_ROOT}/embedded/ui/Dashboard.cpp"
 "${CLUSTER_ROOT}/embedded/ui/Icons.cpp"
 ${CLUSTER_COMPONENTS} ${CLUSTER_FONTS}
 "${CLUSTER_ROOT}/assets/icons/lvgl/cluster_icons_32.c"
 "${CLUSTER_ROOT}/assets/icons/lvgl/cluster_icons_30.c"
 "${CLUSTER_ROOT}/assets/animations/lvgl/r15_signature.c"
 INCLUDE_DIRS "${CLUSTER_ROOT}" "../include"
 REQUIRES lvgl esp_lcd esp_driver_gpio esp_driver_i2c esp_timer esp_psram nvs_flash)
target_compile_features(${COMPONENT_LIB} PUBLIC cxx_std_17)
target_compile_options(${COMPONENT_LIB} PRIVATE -Wall -Wextra)
