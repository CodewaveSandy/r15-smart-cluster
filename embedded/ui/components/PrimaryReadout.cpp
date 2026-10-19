#include "PrimaryReadout.h"
#include "../WidgetPrimitives.h"
#include "../InstrumentPresentation.h"
#include <cmath>
#include <cstdio>

namespace cluster::ui {
using namespace widget;
using namespace geometry;
PrimaryReadout::PrimaryReadout(lv_obj_t* parent) {
    speed_.reset(label(parent,ReadoutLayout::streetSpeed,"—",fonts::speed));
    lv_obj_set_style_text_align(speed_.get(),LV_TEXT_ALIGN_CENTER,0);
    unit_.reset(label(parent,ReadoutLayout::streetUnit,"km/h",fonts::unit));
    lv_obj_set_style_text_align(unit_.get(),LV_TEXT_ALIGN_CENTER,0);
    lap_.reset(label(parent,ReadoutLayout::lap,"—:—.—",fonts::track));
    lv_obj_set_style_text_align(lap_.get(),LV_TEXT_ALIGN_CENTER,0);
    lapHint_.reset(label(parent,ReadoutLayout::lapCaption,"CURRENT LAP"));
    lv_obj_set_style_text_align(lapHint_.get(),LV_TEXT_ALIGN_CENTER,0);
    speedTitle_.reset(label(parent,ReadoutLayout::speedCaption,"SPEED"));
    lv_obj_set_style_text_align(speedTitle_.get(),LV_TEXT_ALIGN_CENTER,0);
}
void PrimaryReadout::update(const ClusterState& s,const ViewOptions& v,const Palette& p) {
    ink(unit_.get(),p.muted);
    char b[32];
    if(s.speedKph.available()) std::snprintf(b,sizeof b,"%d",int(std::lround(s.speedKph.value*(v.imperial?0.621371:1)))); else std::snprintf(b,sizeof b,"—"); text(speed_.get(),b);
    if(!initialized_ || previousTrack_!=v.track) {
        lv_obj_set_style_text_font(speed_.get(),v.track?fonts::secondary:fonts::speed,0);
        place(speed_.get(),v.track?ReadoutLayout::trackSpeed:ReadoutLayout::streetSpeed);
        place(unit_.get(),v.track?ReadoutLayout::trackUnit:ReadoutLayout::streetUnit);
    }
    text(unit_.get(),v.imperial?"mph":"km/h");
    show(lap_.get(),v.track); show(lapHint_.get(),v.track); show(speedTitle_.get(),v.track);
    ink(lapHint_.get(),p.accent); ink(speedTitle_.get(),p.muted);
    text(lap_.get(),lapText(s.currentLapMs).value);
    initialized_=true; previousTrack_=v.track;
}
} // namespace cluster::ui
