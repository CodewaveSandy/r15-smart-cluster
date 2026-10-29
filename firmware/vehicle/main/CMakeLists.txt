idf_component_register(SRCS "main.cpp" "BlePhone.cpp" INCLUDE_DIRS "../../.." REQUIRES esp_driver_gpio esp_driver_uart esp_timer bt nvs_flash)
