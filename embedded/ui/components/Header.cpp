#include "Header.h"
#include "../WidgetPrimitives.h"
#include "../Icons.h"
#include <cstdio>

namespace cluster::ui {
using namespace widget;
using namespace geometry;
Header::Header(lv_obj_t* parent) {
    header_.reset(box(parent,HeaderLayout::bounds));
    gear_=label(header_.get(),HeaderLayout::gear,"—",fonts::gear);
    mode_=label(header_.get(),HeaderLayout::mode,"STREET",fonts::heading);
    subtitle_=label(header_.get(),HeaderLayout::subtitle,"ROAD / EVERYDAY");
    phone_=createIcon(header_.get(),HeaderLayout::phone.x,HeaderLayout::phone.y,Icon::Phone);
    phoneBattery_=label(header_.get(),HeaderLayout::phoneBattery,"PHONE  —");
    qs_=label(header_.get(),HeaderLayout::qs,"QS"); vva_=label(header_.get(),HeaderLayout::vva,"VVA");
    clock_=label(header_.get(),HeaderLayout::clock,"—:—",fonts::clock); alignRight(clock_);
    date_=label(header_.get(),HeaderLayout::date,"CLOCK UNAVAILABLE",fonts::caption); alignRight(date_);
    rules_[0].reset(box(parent,HeaderLayout::divider)); rules_[1].reset(box(parent,HeaderLayout::gearDivider));
}
void Header::update(const ClusterState& s,const ViewOptions& v,const Palette& p) {
    for(const auto& r:rules_) paint(r.get(),p.line);
    ink(date_,p.muted);
    char b[96];
    if(s.gear.available()) { if(s.gear.value==0) std::snprintf(b,sizeof b,"N"); else std::snprintf(b,sizeof b,"%d",s.gear.value); } else std::snprintf(b,sizeof b,"—");
    text(gear_,b); ink(gear_,s.gear.available()&&s.gear.value==0?p.accent:p.ink);
    text(mode_,v.track?"TRACK":"STREET");
    const bool unavailable=!s.speedKph.available()||!s.engineRpm.available()||!s.gear.available();
    text(subtitle_,v.iconTest?"ICON TEST":unavailable?"DATA UNAVAILABLE":v.track?"LAP / PERFORMANCE":"ROAD / EVERYDAY");
    ink(subtitle_,unavailable?p.amber:p.muted);
    text(clock_,v.time); text(date_,v.date);
    show(qs_,v.iconTest||(s.quickShifter.available()&&s.quickShifter.value)); ink(qs_,p.accent);
    show(vva_,v.iconTest||(s.vva.available()&&s.vva.value)); ink(vva_,p.amber);
    statusLamp(phone_,s.phoneConnected,true,p.accent,p,v.iconTest);
    if(s.phoneConnected.available()&&s.phoneConnected.value&&s.phoneBatteryPercent.available()) std::snprintf(b,sizeof b,"PHONE %d%%",s.phoneBatteryPercent.value);
    else std::snprintf(b,sizeof b,"PHONE —"); text(phoneBattery_,b); ink(phoneBattery_,p.muted);
}
} // namespace cluster::ui
