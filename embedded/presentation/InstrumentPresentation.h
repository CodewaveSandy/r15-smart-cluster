#pragma once
#include "shared/model/ClusterState.h"
#include "PresentationPolicy.h"
#include "embedded/application/Lifecycle.h"
#include <algorithm>
#include <cstdio>
namespace cluster::presentation {
inline bool instrumentationVisible(const Lifecycle& life,bool urgentWarning) {
    const auto phase=life.phase();
    const bool animation=phase==Phase::Boot||phase==Phase::Shutdown;
    return phase==Phase::Running || (animation&&urgentWarning);
}
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
struct FuelPresentation { int segments; FuelSeverity severity; bool lit; };
inline FuelPresentation fuelPresentation(const Signal<int>& fuel,std::uint64_t now) {
    if(!fuel.available()) return {0,FuelSeverity::Unknown,true};
    const int value=std::clamp(fuel.value,0,100);
    return {(value*Policy::fuelSegments+99)/100,value<10?FuelSeverity::Critical:value<=20?FuelSeverity::Warning:FuelSeverity::Normal,
        value>=20 || (now/Policy::fuelPeriod)%2==0};
}
inline bool shiftLit(const Signal<int>& rpm,std::uint64_t now) {
    return rpm.available() && rpm.value>=Policy::shiftRpm &&
        (rpm.value<Policy::shiftFlashRpm || (now/Policy::shiftPeriod)%2==0);
}
}
