#pragma once
#include "Typography.h"
#include "Tokens.h"
#include "Geometry.h"
#include <cstring>
#include "embedded/presentation/DashboardPresentation.h"

// Small LVGL construction/mutation operations shared by the dashboard components.
// No transport, formatting policy, ownership registry or global mutable state.
namespace cluster::ui::widget {
inline lv_color_t color(std::uint32_t c) { return lv_color_hex(c); }
inline lv_obj_t* box(lv_obj_t* parent,int x,int y,int w,int h) {
    auto* o=lv_obj_create(parent); lv_obj_remove_style_all(o);
    lv_obj_set_pos(o,x,y); lv_obj_set_size(o,w,h);
    lv_obj_remove_flag(o,LV_OBJ_FLAG_SCROLLABLE); return o;
}
inline lv_obj_t* label(lv_obj_t* parent,int x,int y,int w,const char* text,const lv_font_t* font=fonts::label) {
    auto* o=lv_label_create(parent); lv_obj_set_pos(o,x,y); lv_obj_set_width(o,w);
    lv_obj_set_style_text_font(o,font,0); lv_label_set_long_mode(o,LV_LABEL_LONG_CLIP);
    lv_label_set_text(o,text); return o;
}
inline lv_obj_t* box(lv_obj_t* parent,geometry::Rect bounds) {
    return box(parent,bounds.x,bounds.y,bounds.width,bounds.height);
}
inline lv_obj_t* label(lv_obj_t* parent,geometry::TextArea bounds,const char* value,const lv_font_t* font=fonts::label) {
    return label(parent,bounds.x,bounds.y,bounds.width,value,font);
}
inline void place(lv_obj_t* object,geometry::TextArea bounds) {
    lv_obj_set_pos(object,bounds.x,bounds.y);
    lv_obj_set_width(object,bounds.width);
}
inline void text(lv_obj_t* o,const char* s) { if(std::strcmp(lv_label_get_text(o),s)) lv_label_set_text(o,s); }
inline void background(lv_obj_t* o,std::uint32_t c) {
    if(!lv_color_eq(lv_obj_get_style_bg_color(o,0),color(c))) lv_obj_set_style_bg_color(o,color(c),0);
}
inline void paint(lv_obj_t* o,std::uint32_t c) {
    background(o,c);
    if(lv_obj_get_style_bg_opa(o,0)!=LV_OPA_COVER) lv_obj_set_style_bg_opa(o,LV_OPA_COVER,0);
}
inline void ink(lv_obj_t* o,std::uint32_t c) {
    if(!lv_color_eq(lv_obj_get_style_text_color(o,0),color(c))) lv_obj_set_style_text_color(o,color(c),0);
}
inline void show(lv_obj_t* o,bool yes) {
    if(lv_obj_has_flag(o,LV_OBJ_FLAG_HIDDEN)==!yes) return;
    if(yes) lv_obj_remove_flag(o,LV_OBJ_FLAG_HIDDEN); else lv_obj_add_flag(o,LV_OBJ_FLAG_HIDDEN);
}
inline void fade(lv_obj_t* o,lv_opa_t opacity) {
    if(lv_obj_get_style_opa(o,0)!=opacity) lv_obj_set_style_opa(o,opacity,0);
}
inline void alignRight(lv_obj_t* o) { lv_obj_set_style_text_align(o,LV_TEXT_ALIGN_RIGHT,0); }
inline void statusLamp(lv_obj_t* o,const Signal<bool>& signal,bool phase,std::uint32_t active,const Palette& p,bool iconTest=false) {
    const auto presentation=presentation::lampPresentation(signal,phase,iconTest);
    const auto tone=presentation.tone;
    ink(o,tone==presentation::LampTone::Active?active:tone==presentation::LampTone::Unknown?p.amber:p.muted);
    if(lv_obj_get_style_opa(o,0)!=presentation.opacity) lv_obj_set_style_opa(o,presentation.opacity,0);
}
} // namespace cluster::ui::widget
