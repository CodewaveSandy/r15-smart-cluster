#pragma once
#include "lvgl.h"
#include "shared/application/MapGesture.h"

// Board-only adapter. Application/LVGL calls stay on app_main's task.
namespace cluster::board {
void initialize();
void backlight(bool enabled);
bool readTouch(maps::Touch&);
unsigned touchFailures();
unsigned flushCount();
unsigned maxFlushUs();
}
