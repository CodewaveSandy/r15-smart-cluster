#pragma once
#include "shared/model/ClusterState.h"
#include "Tokens.h"
#include <algorithm>
namespace cluster::ui {
enum class FuelSeverity { Normal, Warning, Critical, Unknown };
struct FuelPresentation { int segments; FuelSeverity severity; bool lit; };
inline FuelPresentation fuelPresentation(const Signal<int>& fuel,std::uint64_t now) {
    if(!fuel.available()) return {0,FuelSeverity::Unknown,true};
    const int value=std::clamp(fuel.value,0,100);
    return {(value*8+99)/100,value<10?FuelSeverity::Critical:value<=20?FuelSeverity::Warning:FuelSeverity::Normal,
        value>=20 || (now/Metrics::fuelPeriod)%2==0};
}
inline bool shiftLit(const Signal<int>& rpm,std::uint64_t now) {
    return rpm.available() && rpm.value>=Metrics::shiftRpm &&
        (rpm.value<Metrics::shiftFlashRpm || (now/Metrics::shiftPeriod)%2==0);
}
}
