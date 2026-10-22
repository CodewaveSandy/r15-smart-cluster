#include "Header.h"
#include "../WidgetPrimitives.h"
#include "../Icons.h"
#include "embedded/presentation/DashboardPresentation.h"

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
void Header::update(const ClusterState& s,const ViewOptions& v,const Palette& p,int settingsProgress) {
    for(auto* label:{mode_,subtitle_,qs_,vva_}) fade(label,255-settingsProgress*255/1000);
    for(const auto& r:rules_) paint(r.get(),p.line);
    ink(date_,p.ink);
    const bool gearChanged=lastGear_.update(s.gear);
    const bool batteryChanged=lastPhoneBattery_.update(s.phoneBatteryPercent);
    const bool connectionChanged=lastPhoneConnection_.update(s.phoneConnected);
    // Header status and badges are cheap; numeric text is formatted only when
    // its input changes. The presenter remains the sole formatting authority.
    const auto presentation=presentation::headerStatus(s,v,v.iconTest);
    if(gearChanged) gearText_.update(gear_,presentation::gearText(s.gear).value);
    ink(gear_,presentation.neutral?p.accent:p.ink);
    text(mode_,presentation.mode); text(subtitle_,presentation.subtitle);
    ink(subtitle_,presentation.unavailable?p.amber:p.ink);
    text(clock_,v.time); text(date_,v.date);
    show(qs_,presentation.quickShifter); ink(qs_,p.accent);
    show(vva_,presentation.vva); ink(vva_,p.amber);
    statusLamp(phone_,s.phoneConnected,true,p.accent,p,v.iconTest);
    if(batteryChanged||connectionChanged) batteryText_.update(phoneBattery_,presentation::phoneBatteryText(s.phoneConnected,s.phoneBatteryPercent).value);
    ink(phoneBattery_,p.ink);
}
} // namespace cluster::ui
