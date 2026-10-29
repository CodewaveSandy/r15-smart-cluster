#include "ClockPanel.h"
#include "../WidgetPrimitives.h"
#include <cstdio>
#include <cstdlib>
namespace cluster::ui {
using namespace widget;
using namespace geometry;
ClockPanel::ClockPanel(lv_obj_t* parent):root_(box(parent,ClockLayout::bounds)) {
    title_=label(root_.get(),SettingsLayout::title,"CLOCK & DATE",fonts::heading);
    back_=label(root_.get(),SettingsLayout::back,"< Back",fonts::value);alignRight(back_);
    for(unsigned i=0;i<7;++i) {
        const auto bounds=i<5?ClockLayout::row(i):i==5?AppearanceLayout::cancel:AppearanceLayout::apply;
        rows_[i]=box(root_.get(),bounds);lv_obj_set_style_radius(rows_[i],6,0);lv_obj_set_style_border_width(rows_[i],2,0);
        labels_[i]=label(rows_[i],16,10,bounds.width-32,"",fonts::value);
    }
    status_=label(root_.get(),AppearanceLayout::status,"",fonts::label);
    show(root_.get(),false);
}
void ClockPanel::update(const ViewOptions& v,const Palette& p,int progress) {
    show(root_.get(),v.clock&&progress==1000);if(!v.clock||progress!=1000)return;
    ink(title_,p.ink);ink(back_,p.muted);
    char format[64],date[64],zone[64];
    std::snprintf(format,sizeof format,"Time format: %s",v.clockPreferences.twelveHour?"12 hour (AM / PM)":"24 hour");
    const char* patterns[]={"DD/MM/YYYY","MM/DD/YYYY","YYYY-MM-DD"};
    std::snprintf(date,sizeof date,"Date format: %s",patterns[unsigned(v.clockPreferences.dateFormat)%3]);
    const int offset=v.clockPreferences.automaticTimezone?v.clockSourceOffset:v.clockPreferences.utcOffsetMinutes;
    std::snprintf(zone,sizeof zone,"Timezone: %s",v.clockPreferences.automaticTimezone?"Follow Mac":"Manual");
    std::snprintf(zone,sizeof zone,"Timezone: %s UTC%c%02d:%02d",v.clockPreferences.automaticTimezone?"Mac":"Manual",offset<0?'-':'+',std::abs(offset)/60,std::abs(offset)%60);
    const char* textValues[]={format,date,zone,"Timezone -15 minutes","Timezone +15 minutes","Cancel","Apply"};
    for(unsigned i=0;i<7;++i) {
        paint(rows_[i],p.panel);text(labels_[i],textValues[i]);ink(labels_[i],p.ink);
        const auto border=color(i==v.clockFocus?p.accent:p.line);
        if(!lv_color_eq(lv_obj_get_style_border_color(rows_[i],0),border))lv_obj_set_style_border_color(rows_[i],border,0);
    }
    text(status_,v.clockSaving?"Saving...":v.clockSaveFailed?"Could not save. Try Apply again.":"Preview changes, then Apply");
    ink(status_,v.clockSaveFailed?p.red:p.muted);
}
}
