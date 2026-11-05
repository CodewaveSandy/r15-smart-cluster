#pragma once
#include "lvgl.h"
#include "shared/model/ClusterState.h"
#include "embedded/application/Lifecycle.h"
#include "Tokens.h"
#include "components/Header.h"
#include "components/SettingsHome.h"
#include "components/AppearancePanel.h"
#include "components/PhonePanel.h"
#include "components/ClockPanel.h"
#include "embedded/presentation/SettingsTransition.h"
#include "components/TellTaleRow.h"
#include "components/Tachometer.h"
#include "components/PrimaryReadout.h"
#include "components/RideInfoPanel.h"
#include "components/FuelGauge.h"
#include "components/TurnGuidanceCard.h"
#include "components/LifecycleOverlay.h"
#include "TouchNavigation.h"

namespace cluster::ui {
// Owns the root and fixed-size component members. Components are destroyed
// before their parent root; each owns its top-level nodes and LVGL owns their
// descendants. A persistent instrument group supports visibility switching;
// settings and its transition add no tasks or per-frame application allocations.
// The caller's parent must outlive Dashboard.
// Construct/update/destroy only on the LVGL owner thread.
class Dashboard {
public:
    explicit Dashboard(lv_obj_t* parent);
    ~Dashboard();
    Dashboard(const Dashboard&)=delete;
    Dashboard& operator=(const Dashboard&)=delete;
    void guidance(const guidance::TurnGuidanceState& value){guidanceState_=value;}
    bool guidanceVisible()const{return guidanceVisible_;}
    void update(const ClusterState&,const ViewOptions&,const Lifecycle&,std::uint64_t now);
    std::optional<SettingsEffect> touch(ViewOptions& view,const ClusterState& state,const Lifecycle& life,int x,int y) {
        if(!touchVisible_||life.phase()!=Phase::Running) return std::nullopt;
        return settingsTouch(view,state,life,x,y,touchMenuReady_);
    }
private:
    lv_grad_dsc_t shiftGradient_{};
    ObjectOwner root_;
    ObjectOwner instruments_;
    presentation::SettingsTransition settingsTransition_;
    Header header_;
    TellTaleRow tellTales_;
    Tachometer tachometer_;
    SettingsHome settings_;
    AppearancePanel appearance_;
    PhonePanel phone_;
    ClockPanel clock_;
    PrimaryReadout primary_;
    RideInfoPanel rideInfo_;
    FuelGauge fuel_;
    TurnGuidanceCard guidance_;guidance::TurnGuidanceState guidanceState_;bool guidanceVisible_=false;
    LifecycleOverlay overlay_;
    bool initialized_=false, previousLight_=false, previousShift_=false;
    bool touchVisible_=false,touchMenuReady_=false;
};
} // namespace cluster::ui
