#pragma once
// Dashboard feature profile validated against pinned LVGL 9.2.2 on the host.
// Board ports must select their own OS, allocator, DMA and display buffers.
// Retain software gradients/triangles/antialiased text and warning diagnostics.
#define LV_USE_ANIMIMG 0
#define LV_USE_ARC 0
#define LV_USE_BAR 0
#define LV_USE_BUTTON 0
#define LV_USE_BUTTONMATRIX 0
#define LV_USE_CALENDAR 0
#define LV_USE_CANVAS 0
#define LV_USE_CHART 0
#define LV_USE_CHECKBOX 0
#define LV_USE_DROPDOWN 0
#define LV_USE_IMAGE 0
#define LV_USE_IMAGEBUTTON 0
#define LV_USE_KEYBOARD 0
#define LV_USE_LABEL 1
#define LV_USE_LED 0
#define LV_USE_LINE 0
#define LV_USE_LIST 0
#define LV_USE_LOTTIE 0
#define LV_USE_MENU 0
#define LV_USE_MSGBOX 0
#define LV_USE_ROLLER 0
#define LV_USE_SCALE 0
#define LV_USE_SLIDER 0
#define LV_USE_SPAN 0
#define LV_USE_SPINBOX 0
#define LV_USE_SPINNER 0
#define LV_USE_SWITCH 0
#define LV_USE_TEXTAREA 0
#define LV_USE_TABLE 0
#define LV_USE_TABVIEW 0
#define LV_USE_TILEVIEW 0
#define LV_USE_WIN 0
#define LV_USE_FLEX 0
#define LV_USE_GRID 0
#define LV_USE_OBSERVER 0
#define LV_USE_THEME_SIMPLE 0
#define LV_USE_THEME_MONO 0
// The default theme still supplies existing label styles. Every instrument
// sets its role font explicitly; the base font reuses our bundled Inter asset.
#define LV_FONT_MONTSERRAT_14 0
#define LV_FONT_CUSTOM_DECLARE LV_FONT_DECLARE(inter_label_14)
#define LV_FONT_DEFAULT (&inter_label_14)
