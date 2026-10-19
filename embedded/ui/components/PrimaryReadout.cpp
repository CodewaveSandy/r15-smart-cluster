#include "PrimaryReadout.h"
#include "../WidgetPrimitives.h"
#include "embedded/presentation/DashboardPresentation.h"

namespace cluster::ui {
using namespace cluster::presentation;
using namespace widget;
using namespace geometry;
namespace {
// Instrument strings contain ASCII digits/punctuation and the UTF-8 unavailable dash.
// Center ink bounds rather than italic advance widths; return the visible right edge.
int centerInstrument(lv_obj_t* label,const lv_font_t* font,TextArea area) {
    const auto* value=lv_label_get_text(label);
    const auto codeAt=[&](unsigned i) -> unsigned {
        return static_cast<unsigned char>(value[i])>=128?8212:static_cast<unsigned char>(value[i]);
    };
    int advance=0,left=0,right=0;
    for(unsigned i=0;value[i];) {
        const unsigned code=codeAt(i);
        const unsigned nextIndex=i+(code==8212?3:1);
        lv_font_glyph_dsc_t glyph{};
        if(lv_font_get_glyph_dsc(font,&glyph,code,codeAt(nextIndex))) {
            if(i==0) left=glyph.ofs_x;
            right=advance+glyph.ofs_x+glyph.box_w;
            advance+=glyph.adv_w;
        }
        i=nextIndex;
    }
    const int origin=area.x+area.width/2-(left+right)/2;
    lv_obj_set_x(label,origin);
    return origin+right;
}
}
PrimaryReadout::PrimaryReadout(lv_obj_t* parent) {
    speed_.reset(label(parent,ReadoutLayout::streetSpeed,"—",fonts::speed));
    lv_obj_set_style_text_align(speed_.get(),LV_TEXT_ALIGN_LEFT,0);
    unit_.reset(label(parent,ReadoutLayout::streetUnit,"km/h",fonts::unit));
    lv_obj_set_style_text_align(unit_.get(),LV_TEXT_ALIGN_LEFT,0);
    lap_.reset(label(parent,ReadoutLayout::lap,"—:—.—",fonts::track));
    lv_obj_set_style_text_align(lap_.get(),LV_TEXT_ALIGN_LEFT,0);
    lapHint_.reset(label(parent,ReadoutLayout::lapCaption,"CURRENT LAP"));
    lv_obj_set_style_text_align(lapHint_.get(),LV_TEXT_ALIGN_CENTER,0);
    speedTitle_.reset(label(parent,ReadoutLayout::speedCaption,"SPEED"));
    lv_obj_set_style_text_align(speedTitle_.get(),LV_TEXT_ALIGN_CENTER,0);
}
void PrimaryReadout::update(const ClusterState& s,const ViewOptions& v,const Palette& p) {
    ink(unit_.get(),p.muted);
    const bool speedChanged=lastSpeed_.update(s.speedKph);
    const bool lapChanged=lastLap_.update(s.currentLapMs);
    if(!initialized_ || speedChanged || (v.track&&lapChanged) || previousImperial_!=v.imperial || previousTrack_!=v.track) {
        const auto presentation=primaryPresentation(s,v);
        text(speed_.get(),presentation.speed.value);
        if(v.track) text(lap_.get(),presentation.lap.value);
    }
    if(!initialized_ || previousTrack_!=v.track) {
        lv_obj_set_style_text_font(speed_.get(),v.track?fonts::secondary:fonts::speed,0);
        place(speed_.get(),v.track?ReadoutLayout::trackSpeed:ReadoutLayout::streetSpeed);
        place(unit_.get(),v.track?ReadoutLayout::trackUnit:ReadoutLayout::streetUnit);
    }
    text(unit_.get(),v.imperial?"mph":"km/h");
    if(!initialized_ || speedChanged || previousTrack_!=v.track || previousImperial_!=v.imperial) {
        const auto area=v.track?ReadoutLayout::trackSpeed:ReadoutLayout::streetSpeed;
        const auto unitArea=v.track?ReadoutLayout::trackUnit:ReadoutLayout::streetUnit;
        const int right=centerInstrument(speed_.get(),v.track?fonts::secondary:fonts::speed,area);
        lv_obj_set_pos(unit_.get(),right+10,unitArea.y);
    }
    if(v.track && (!initialized_ || lapChanged || previousTrack_!=v.track))
        centerInstrument(lap_.get(),fonts::track,ReadoutLayout::lap);

    show(lap_.get(),v.track); show(lapHint_.get(),v.track); show(speedTitle_.get(),v.track);
    ink(lapHint_.get(),p.accent); ink(speedTitle_.get(),p.muted);
    initialized_=true; previousTrack_=v.track; previousImperial_=v.imperial;
}
} // namespace cluster::ui
