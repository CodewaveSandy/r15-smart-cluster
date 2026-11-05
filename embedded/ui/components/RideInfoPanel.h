#pragma once
#include "lvgl.h"
#include "../ObjectOwner.h"
#include "../ReadoutText.h"
#include "embedded/presentation/SignalChange.h"
#include "../Tokens.h"
#include "shared/model/ClusterState.h"

namespace cluster::ui {
// Owns its top-level LVGL objects and their descendants. The supplied parent
// must outlive this component. All operations run on the LVGL owner thread.
class RideInfoPanel {
public:
    explicit RideInfoPanel(lv_obj_t* parent);
    RideInfoPanel(const RideInfoPanel&)=delete;
    RideInfoPanel& operator=(const RideInfoPanel&)=delete;
    void visible(bool value){visible_=value;}
    void update(const ClusterState&,const ViewOptions&,const Palette&);
private:
    bool visible_=true,distanceVisible_=false;
    ReadoutText tripText_, infoText_;
    presentation::SignalChange<double> lastDistance_,lastInfoValue_;
    presentation::SignalChange<std::uint32_t> lastBestLap_;
    presentation::SignalChange<bool> lastTcs_;
    bool initialized_=false,previousTrack_=false,previousImperial_=false;
    TripPage previousTrip_=TripPage::Odometer;
    InfoPage previousInfo_=InfoPage::Coolant;
    ObjectOwner lowerRule_, tripTitle_, tripValue_, tripUnit_, infoTitle_, infoValue_;
};
} // namespace cluster::ui
