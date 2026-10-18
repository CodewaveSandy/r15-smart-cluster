# System architecture

Baseline: 2026-10-18. Design stage.

```text
Bike harness -> reviewed conditioning / CAN transceiver
                              |
                       Vehicle ESP32
          acquisition, validation, calculations, Wi-Fi/BLE
                              |
                versioned normalized UART messages
                              |
                      Display ESP32-S3
               state, navigation, LVGL rendering
                              |
                      800 x 480 display
```

The display is an Elecrow 7-inch HMI with reported ESP32-S3-WROOM-1-N4R8, 4 MB flash and 8 MB PSRAM. Exact PCB revision and the vehicle MCU model must be confirmed. The [user-supplied vendor reference](https://www.elecrow.com/wiki/esp32-display-702727-intelligent-touch-screen-wi-fi26ble-800480-hmi-display.html) identifies the product family.

Vehicle protocols, calibration and calculations belong to the vehicle MCU. Display widgets consume normalized observations with explicit units, quality and freshness. No raw CAN frames or harness pins enter UI code. Physical inputs become semantic navigation actions. Display preferences remain local; vehicle-owned odometer handling needs a separate verified contract.

Proposed implementation: ESP-IDF, FreeRTOS, C++17, LVGL and an SDL host preview using the same UI source. Versions will be pinned after board-example validation. Wi-Fi/BLE is not evidence of phone projection capability. Full Maps/Android Auto/CarPlay remains unresolved and requires a separate feasibility investigation.

See [embedded design](embedded-architecture.md), [style guide](lvgl-style-guide.md) and [power lifecycle](power-lifecycle.md). Power architecture must support ignition sensing, startup stability and retained shutdown power; no circuit or timing guarantee is currently verified.
