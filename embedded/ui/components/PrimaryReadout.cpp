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
int centerInstrument(lv_obj_t* label,const lv_font_t* font,TextArea area,int scale=256,const char* displayed=nullptr) {
    const auto* value=displayed?displayed:lv_label_get_text(label);
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
    const int origin=area.x+area.width/2-(left+right)*scale/512;
    lv_obj_set_x(label,origin);
    return origin+right*scale/256;
}
}
PrimaryReadout::PrimaryReadout(lv_obj_t* parent) {
    speed_.reset(label(parent,ReadoutLayout::streetSpeed,"—",fonts::speed));
    lv_obj_set_style_text_align(speed_.get(),LV_TEXT_ALIGN_LEFT,0);
    leadingZeros_.reset(label(parent,ReadoutLayout::streetSpeed,"",fonts::speed));
    lv_obj_set_style_text_align(leadingZeros_.get(),LV_TEXT_ALIGN_LEFT,0);
    fade(leadingZeros_.get(),Metrics::speedPlaceholderOpacity);
    unit_.reset(label(parent,ReadoutLayout::streetUnit,"km/h",fonts::unit));
    lv_obj_set_style_text_align(unit_.get(),LV_TEXT_ALIGN_LEFT,0);
    lap_.reset(label(parent,ReadoutLayout::lap,"—:—.—",fonts::track));
    lv_obj_set_style_text_align(lap_.get(),LV_TEXT_ALIGN_LEFT,0);
    lapHint_.reset(label(parent,ReadoutLayout::lapCaption,"CURRENT LAP"));
    lv_obj_set_style_text_align(lapHint_.get(),LV_TEXT_ALIGN_CENTER,0);
    speedTitle_.reset(label(parent,ReadoutLayout::speedCaption,"SPEED"));
    lv_obj_set_style_text_align(speedTitle_.get(),LV_TEXT_ALIGN_CENTER,0);
}
void PrimaryReadout::update(const ClusterState& s,const ViewOptions& v,const Palette& p,int settingsProgress) {
    ink(unit_.get(),p.ink);
    ink(leadingZeros_.get(),p.ink);
    const bool speedChanged=lastSpeed_.update(s.speedKph);
    const bool lapChanged=lastLap_.update(s.currentLapMs);
    if(!initialized_ || speedChanged || (v.track&&lapChanged) || previousImperial_!=v.imperial || previousTrack_!=v.track) {
        const auto presentation=primaryPresentation(s,v);
        speedText_.update(speed_.get(),presentation.speed.value);
        if(v.track) lapText_.update(lap_.get(),presentation.lap.value);
    }
    if(!initialized_ || previousTrack_!=v.track) {
        lv_obj_set_style_text_font(speed_.get(),v.track?fonts::secondary:fonts::speed,0);
        lv_obj_set_style_text_font(leadingZeros_.get(),v.track?fonts::secondary:fonts::speed,0);
        place(leadingZeros_.get(),v.track?ReadoutLayout::trackSpeed:ReadoutLayout::streetSpeed);
        place(speed_.get(),v.track?ReadoutLayout::trackSpeed:ReadoutLayout::streetSpeed);
        place(unit_.get(),v.track?ReadoutLayout::trackUnit:ReadoutLayout::streetUnit);
    }
    const int scale=v.track?256:256+(92-256)*settingsProgress/1000;
    if(!initialized_ || previousProgress_!=settingsProgress || previousTrack_!=v.track) {
        lv_obj_set_style_transform_pivot_x(leadingZeros_.get(),0,0);
        lv_obj_set_style_transform_pivot_y(leadingZeros_.get(),0,0);
        lv_obj_set_style_transform_scale_x(leadingZeros_.get(),scale,0);
        lv_obj_set_style_transform_scale_y(leadingZeros_.get(),scale,0);
        lv_obj_set_style_transform_pivot_x(speed_.get(),0,0);
        lv_obj_set_style_transform_pivot_y(speed_.get(),0,0);
        lv_obj_set_style_transform_scale_x(speed_.get(),scale,0);
        lv_obj_set_style_transform_scale_y(speed_.get(),scale,0);
    }
    text(unit_.get(),v.imperial?"mph":"km/h");
    if(!initialized_ || speedChanged || previousTrack_!=v.track || previousImperial_!=v.imperial || previousProgress_!=settingsProgress) {
        auto area=v.track?ReadoutLayout::trackSpeed:ReadoutLayout::streetSpeed;
        const int center=area.x+area.width/2;
        area.x+=(SettingsLayout::speedCenter-center)*settingsProgress/1000;
        area.y+=(SettingsLayout::speedY-area.y)*settingsProgress/1000;
        lv_obj_set_y(speed_.get(),area.y);
        const auto unitArea=v.track?ReadoutLayout::trackUnit:ReadoutLayout::streetUnit;
        const auto* font=v.track?fonts::secondary:fonts::speed;
        const auto* digits=lv_label_get_text(speed_.get());
        const auto length=std::strlen(digits);
        const unsigned zeros=s.speedKph.available()&&length<3?3-unsigned(length):0;
        char padded[32]{};
        std::snprintf(padded,sizeof padded,"%s%s",zeros==2?"00":zeros==1?"0":"",digits);
        text(leadingZeros_.get(),zeros==2?"00":zeros==1?"0":"");
        const int right=centerInstrument(speed_.get(),font,area,scale,padded);
        lv_obj_set_pos(leadingZeros_.get(),lv_obj_get_style_x(speed_.get(),0),area.y);
        int prefixAdvance=0;
        for(unsigned i=0;i<zeros;++i) {
            lv_font_glyph_dsc_t glyph{};
            if(lv_font_get_glyph_dsc(font,&glyph,'0',i+1<zeros?'0':unsigned(digits[0]))) prefixAdvance+=glyph.adv_w;
        }
        lv_obj_set_x(speed_.get(),lv_obj_get_style_x(speed_.get(),0)+prefixAdvance*scale/256);
        lv_obj_set_pos(unit_.get(),right+10,unitArea.y+(SettingsLayout::unitY-unitArea.y)*settingsProgress/1000);
    }
    if(v.track && (!initialized_ || lapChanged || previousTrack_!=v.track))
        centerInstrument(lap_.get(),fonts::track,ReadoutLayout::lap);

    for(auto* label:{lap_.get(),lapHint_.get(),speedTitle_.get()}) fade(label,255-settingsProgress*255/1000);
    show(lap_.get(),v.track&&settingsProgress==0); show(lapHint_.get(),v.track&&settingsProgress==0); show(speedTitle_.get(),v.track&&settingsProgress==0);
    ink(lapHint_.get(),p.ink); ink(speedTitle_.get(),p.ink);
    initialized_=true; previousTrack_=v.track; previousImperial_=v.imperial; previousProgress_=settingsProgress;
}
} // namespace cluster::ui
