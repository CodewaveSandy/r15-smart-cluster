#pragma once
#define LV_CONF_H
#define LV_COLOR_DEPTH 16
// Service moving dirty areas at a 16 ms cadence instead of LVGL's 33 ms
// default. Scanout/frame-complete waits still bound actual rendered updates.
#define LV_DEF_REFR_PERIOD 16
#define LV_USE_OS LV_OS_NONE
#define LV_USE_STDLIB_MALLOC LV_STDLIB_CLIB
#define LV_USE_STDLIB_STRING LV_STDLIB_CLIB
#define LV_USE_STDLIB_SPRINTF LV_STDLIB_CLIB
#define LV_USE_LOG 1
#define LV_LOG_PRINTF 1
#define LV_LOG_LEVEL LV_LOG_LEVEL_WARN
#define LV_BUILD_EXAMPLES 0
#define LV_BUILD_DEMOS 0
#define LV_USE_THORVG_INTERNAL 0
#define LV_USE_SYSMON 0
#define LV_USE_PERF_MONITOR 0
#define LV_USE_MEM_MONITOR 0
#define LV_GRADIENT_MAX_STOPS 6
#include "../../../embedded/ui/LvglFeatures.h"
