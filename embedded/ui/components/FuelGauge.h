#pragma once
#include "lvgl.h"
#include "../ObjectOwner.h"
#include "../Tokens.h"
#include "shared/model/ClusterState.h"
#include <array>

namespace cluster::ui {
// Owns its top-level LVGL objects and their descendants. The supplied parent
// must outlive this component. All operations run on the LVGL owner thread.
class FuelGauge {
public:
    explicit FuelGauge(lv_obj_t* parent);
    FuelGauge(const FuelGauge&)=delete;
    FuelGauge& operator=(const FuelGauge&)=delete;
    void update(const Signal<int>& fuel,const Palette&,std::uint64_t now);
private:
    ObjectOwner fuelTitle_, fuelValue_, pump_, fuelDivider_;
    std::array<ObjectOwner,Metrics::fuelSegmentCount> fuel_{};
};
} // namespace cluster::ui
