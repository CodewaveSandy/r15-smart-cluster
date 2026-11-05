#pragma once
#include "lvgl.h"


// Board-only adapter. Application/LVGL calls stay on app_main's task.
namespace cluster::board {
enum class TouchPhase { Press, Move, Release, Cancel };
struct Touch { TouchPhase phase=TouchPhase::Cancel; int x=0,y=0; };
void initialize();
void backlight(bool enabled);
bool readTouch(Touch&);
unsigned touchFailures();
unsigned flushCount();
unsigned maxFlushUs();
}
