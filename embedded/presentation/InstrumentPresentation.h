#pragma once
#include "shared/model/ClusterState.h"
#include "PresentationPolicy.h"
#include <algorithm>
#include <cstdio>
namespace cluster::presentation {
struct LapText { char value[16]{}; };
inline LapText lapText(const Signal<std::uint32_t>& lap) {
    LapText result;
    // The mm:ss.hh field supports 0..99 minutes. Overflow is unavailable, never wrapped.
    if(!lap.available() || lap.value>=100u*60u*1000u) std::snprintf(result.value,sizeof result.value,"—:—.—");
    else std::snprintf(result.value,sizeof result.value,"%02u:%02u.%02u",
        unsigned(lap.value/60000),unsigned(lap.value/1000%60),unsigned(lap.value/10%100));
    return result;
}
enum class FuelSeverity { Normal, Warning, Critical, Unknown };
struct FuelPresentation {
    int percent;
    FuelSeverity severity;
    bool lit, reserve;
    // Each segment holds 100 coverage units: 1% changes eight units in total.
    int segmentFill(int index) const {
        return std::clamp(percent*Policy::fuelSegments-index*100,0,100);
    }
};
inline FuelPresentation fuelPresentation(const Signal<int>& fuel,std::uint64_t now) {
    if(!fuel.available()) return {0,FuelSeverity::Unknown,true,false};
    const int value=std::clamp(fuel.value,0,100);
    return {value,value<10?FuelSeverity::Critical:value<=20?FuelSeverity::Warning:FuelSeverity::Normal,
        value>=20 || (now/Policy::fuelPeriod)%2==0,value<=Policy::fuelReservePercent};
}
inline bool shiftLit(const Signal<int>& rpm,std::uint64_t now) {
    return rpm.available() && rpm.value>=Policy::shiftRpm &&
        (rpm.value<Policy::shiftFlashRpm || (now/Policy::shiftPeriod)%2==0);
}
}
