#pragma once
#include "lvgl.h"
#include "shared/model/ClusterState.h"
#include "embedded/application/Lifecycle.h"
#include "Tokens.h"
#include "components/Header.h"
#include "components/TellTaleRow.h"
#include "components/Tachometer.h"
#include "components/PrimaryReadout.h"
#include "components/RideInfoPanel.h"
#include "components/FuelGauge.h"
#include "components/LifecycleOverlay.h"

namespace cluster::ui {
// Owns the root and fixed-size component members. Components are destroyed
// before their parent root; each owns its top-level nodes and LVGL owns their
// descendants. No extra grouping objects, wrapper allocations or tasks.
// The caller's parent must outlive Dashboard.
// Construct/update/destroy only on the LVGL owner thread.
class Dashboard {
public:
    explicit Dashboard(lv_obj_t* parent);
    ~Dashboard();
    Dashboard(const Dashboard&)=delete;
    Dashboard& operator=(const Dashboard&)=delete;
    void update(const ClusterState&,const ViewOptions&,const Lifecycle&,std::uint64_t now);
private:
    lv_grad_dsc_t shiftGradient_{};
    ObjectOwner root_;
    Header header_;
    TellTaleRow tellTales_;
    Tachometer tachometer_;
    PrimaryReadout primary_;
    RideInfoPanel rideInfo_;
    FuelGauge fuel_;
    LifecycleOverlay overlay_;
    bool initialized_=false, previousLight_=false, previousShift_=false;
};
} // namespace cluster::ui
