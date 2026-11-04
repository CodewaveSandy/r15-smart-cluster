idf_component_register(SRCS "main.cpp" "BlePhone.cpp" INCLUDE_DIRS "../../.." REQUIRES esp_driver_gpio esp_driver_uart esp_driver_rmt esp_timer bt nvs_flash)
target_compile_features(${COMPONENT_LIB} PUBLIC cxx_std_17)
target_compile_options(${COMPONENT_LIB} PRIVATE -Wall -Wextra)
