#include "RideInfoPanel.h"
#include "../WidgetPrimitives.h"
#include "../InstrumentPresentation.h"
#include <cstdio>

namespace cluster::ui {
using namespace widget;
using namespace geometry;
RideInfoPanel::RideInfoPanel(lv_obj_t* parent) {
    lowerRule_.reset(box(parent,RideInfoLayout::divider));
    tripTitle_.reset(label(parent,RideInfoLayout::tripTitle,"ODO"));
    tripValue_.reset(label(parent,RideInfoLayout::tripValue,"—",fonts::heading));
    tripUnit_.reset(label(parent,RideInfoLayout::tripUnit,"km"));
    infoTitle_.reset(label(parent,RideInfoLayout::infoTitle,"COOLANT"));
    infoValue_.reset(label(parent,RideInfoLayout::infoValue,"— V / —°C",fonts::heading));
}
void RideInfoPanel::update(const ClusterState& s,const ViewOptions& v,const Palette& p) {
    ink(tripUnit_.get(),p.muted); ink(infoTitle_.get(),p.muted); ink(tripTitle_.get(),p.accent);
    char b[96];
    paint(lowerRule_.get(),p.line);
    show(tripUnit_.get(),!v.track);
    if(v.track) {
        text(tripTitle_.get(),"BEST LAP");
        text(tripValue_.get(),lapText(s.bestLapMs).value);
    } else {
        text(tripTitle_.get(),tripTitle(v.tripPage));
        const auto distance=tripReading(s,v.tripPage);
        if(distance.available()) std::snprintf(b,sizeof b,"%.1f",distance.value*(v.imperial?0.621371:1));
        else std::snprintf(b,sizeof b,"—");
        text(tripValue_.get(),b); text(tripUnit_.get(),v.imperial?"mi":"km");
    }
    switch(v.infoPage) {
    case InfoPage::Coolant:
        text(infoTitle_.get(),"COOLANT");
        if(s.coolantTemperatureC.available()) std::snprintf(b,sizeof b,"%.0f°C",s.coolantTemperatureC.value);
        else std::snprintf(b,sizeof b,"—°C");
        break;
    case InfoPage::Battery:
        text(infoTitle_.get(),"BATTERY VOLTAGE");
        if(s.batteryVoltageV.available()) std::snprintf(b,sizeof b,"%.1f V",s.batteryVoltageV.value);
        else std::snprintf(b,sizeof b,"— V");
        break;
    case InfoPage::Tcs:
        text(infoTitle_.get(),"TCS"); std::snprintf(b,sizeof b,"%s",s.tcsEnabled.available()?(s.tcsEnabled.value?"ON":"OFF"):"—"); break;
    case InfoPage::Track:
        text(infoTitle_.get(),"TRACK MODE"); std::snprintf(b,sizeof b,"%s",v.track?"ON":"OFF"); break;
    case InfoPage::Count:text(infoTitle_.get(),"—"); std::snprintf(b,sizeof b,"—"); break;
    }
    text(infoValue_.get(),b);
}
} // namespace cluster::ui
