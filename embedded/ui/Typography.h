#pragma once
#include "lvgl.h"
extern "C" {
LV_FONT_DECLARE(inter_caption_12);
LV_FONT_DECLARE(inter_label_14);
LV_FONT_DECLARE(inter_value_16);
LV_FONT_DECLARE(inter_rpm_16);
LV_FONT_DECLARE(inter_unit_18);
LV_FONT_DECLARE(inter_heading_23);
LV_FONT_DECLARE(racing_secondary_46);
LV_FONT_DECLARE(racing_gear_72);
LV_FONT_DECLARE(racing_speed_128);
LV_FONT_DECLARE(racing_track_74);
LV_FONT_DECLARE(barlow_clock_30);
}
namespace cluster::ui::fonts {
inline const lv_font_t* const caption=&inter_caption_12;
inline const lv_font_t* const label=&inter_label_14;
inline const lv_font_t* const value=&inter_value_16;
inline const lv_font_t* const rpm=&inter_rpm_16;
inline const lv_font_t* const unit=&inter_unit_18;
inline const lv_font_t* const heading=&inter_heading_23;
inline const lv_font_t* const secondary=&racing_secondary_46;
inline const lv_font_t* const gear=&racing_gear_72;
inline const lv_font_t* const speed=&racing_speed_128;
inline const lv_font_t* const track=&racing_track_74;
inline const lv_font_t* const clock=&barlow_clock_30;
}
