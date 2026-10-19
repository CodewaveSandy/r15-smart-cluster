#pragma once
#include "lvgl.h"
#include "../ObjectOwner.h"
#include "embedded/presentation/SignalChange.h"
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
    presentation::SignalChange<int> lastFuel_;
    struct SegmentStyle { int fill=0; std::uint32_t active=0, empty=0; };
    static void drawSegment(lv_event_t*);
    ObjectOwner fuelTitle_, fuelValue_, pump_, fuelDivider_, reserveMarker_;
    // Stable callback data; no per-frame allocations or widget reconstruction.
    std::array<SegmentStyle,Metrics::fuelSegmentCount> segmentStyles_{};
    std::array<ObjectOwner,Metrics::fuelSegmentCount> fuel_{};
};
} // namespace cluster::ui
