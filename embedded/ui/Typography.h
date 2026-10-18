#pragma once
#include "lvgl.h"
extern "C" {
LV_FONT_DECLARE(inter_caption_12);
LV_FONT_DECLARE(inter_label_14);
LV_FONT_DECLARE(inter_value_16);
LV_FONT_DECLARE(inter_unit_18);
LV_FONT_DECLARE(inter_heading_23);
LV_FONT_DECLARE(barlow_gear_46);
LV_FONT_DECLARE(barlow_speed_128);
LV_FONT_DECLARE(barlow_track_79);
LV_FONT_DECLARE(barlow_clock_30);
}
namespace cluster::ui::fonts {
inline const lv_font_t* const caption=&inter_caption_12;
inline const lv_font_t* const label=&inter_label_14;
inline const lv_font_t* const value=&inter_value_16;
inline const lv_font_t* const unit=&inter_unit_18;
inline const lv_font_t* const heading=&inter_heading_23;
inline const lv_font_t* const gear=&barlow_gear_46;
inline const lv_font_t* const speed=&barlow_speed_128;
inline const lv_font_t* const track=&barlow_track_79;
inline const lv_font_t* const clock=&barlow_clock_30;
}
