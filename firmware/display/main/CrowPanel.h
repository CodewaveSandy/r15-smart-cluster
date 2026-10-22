#pragma once
#include "lvgl.h"

// Board-only adapter. Application/LVGL calls stay on app_main's task.
namespace cluster::board {
void initialize();
void backlight(bool enabled);
bool releasedTouch(int& x, int& y);
unsigned flushCount();
unsigned maxFlushUs();
}
