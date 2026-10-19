#include "RideInfoPanel.h"
#include "../WidgetPrimitives.h"
#include "embedded/presentation/DashboardPresentation.h"

namespace cluster::ui {
using namespace cluster::presentation;
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
    paint(lowerRule_.get(),p.line);
    bool changed=lastDistance_.update(v.track?Signal<double>{}:tripReading(s,v.tripPage));
    changed=lastBestLap_.update(v.track?s.bestLapMs:Signal<std::uint32_t>{})||changed;
    const auto info=v.infoPage==InfoPage::Coolant?s.coolantTemperatureC:
        v.infoPage==InfoPage::Battery?s.batteryVoltageV:Signal<double>{};
    changed=lastInfoValue_.update(info)||changed;
    changed=lastTcs_.update(v.infoPage==InfoPage::Tcs?s.tcsEnabled:Signal<bool>{})||changed;
    changed=changed||!initialized_||previousTrack_!=v.track||previousImperial_!=v.imperial||previousTrip_!=v.tripPage||previousInfo_!=v.infoPage;
    if(!changed) return;
    initialized_=true; previousTrack_=v.track; previousImperial_=v.imperial;
    previousTrip_=v.tripPage; previousInfo_=v.infoPage;
    const auto presentation=rideInfoPresentation(s,v);
    show(tripUnit_.get(),presentation.showDistance);
    text(tripTitle_.get(),presentation.tripTitle);
    text(tripValue_.get(),presentation.tripValue.value);
    text(tripUnit_.get(),presentation.tripUnit);
    text(infoTitle_.get(),presentation.infoTitle);
    text(infoValue_.get(),presentation.infoValue.value);
}
} // namespace cluster::ui
