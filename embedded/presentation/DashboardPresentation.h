#pragma once
#include "InstrumentPresentation.h"
#include "embedded/application/DisplayOptions.h"
#include <cmath>
#include <cstdio>

namespace cluster::presentation {
// Bounded stack/value output; no strings, allocation, LVGL or platform types.
struct ValueText { char value[32]="—"; };
template<class... Args> inline ValueText formatted(const char* format,Args... args) {
    ValueText result;
    int length;
    if constexpr(sizeof...(Args)==0) length=std::snprintf(result.value,sizeof result.value,"%s",format);
    else length=std::snprintf(result.value,sizeof result.value,format,args...);
    if(length<0 || static_cast<unsigned>(length)>=sizeof result.value)
        std::snprintf(result.value,sizeof result.value,"—");
    return result;
}
inline ValueText rpmText(const Signal<int>& rpm) {
    return rpm.available()?formatted("%d rpm",rpm.value):formatted("— rpm");
}
inline ValueText fuelText(const Signal<int>& fuel) {
    return fuel.available()?formatted("%d%%",fuel.value):ValueText{};
}
inline ValueText gearText(const Signal<int>& gear) {
    if(!gear.available()) return {};
    return gear.value==0?formatted("N"):formatted("%d",gear.value);
}
inline ValueText phoneBatteryText(const Signal<bool>& connected,const Signal<int>& battery) {
    return connected.available()&&connected.value&&battery.available()?formatted("PHONE %d%%",battery.value):formatted("PHONE —");
}
struct HeaderStatus {
    const char* mode;
    const char* subtitle;
    bool neutral,unavailable,quickShifter,vva;
};
inline HeaderStatus headerStatus(const ClusterState& state,const DisplayOptions& options,bool iconTest) {
    const bool unavailable=!state.speedKph.available()||!state.engineRpm.available()||!state.gear.available();
    return {options.track?"TRACK":"STREET",
        iconTest?"ICON TEST":unavailable?"DATA UNAVAILABLE":options.track?"LAP / PERFORMANCE":"ROAD / EVERYDAY",
        state.gear.available()&&state.gear.value==0,unavailable,
        iconTest||(state.quickShifter.available()&&state.quickShifter.value),
        iconTest||(state.vva.available()&&state.vva.value)};
}
struct PrimaryPresentation { ValueText speed; LapText lap; const char* unit; };
inline PrimaryPresentation primaryPresentation(const ClusterState& state,const DisplayOptions& options) {
    PrimaryPresentation result{ValueText{},lapText(state.currentLapMs),options.imperial?"mph":"km/h"};
    if(state.speedKph.available()) result.speed=formatted("%d",int(std::lround(state.speedKph.value*(options.imperial?0.621371:1))));
    return result;
}
struct RideInfoPresentation {
    const char* tripTitle;
    ValueText tripValue;
    const char* tripUnit;
    const char* infoTitle="—";
    ValueText infoValue;
    bool showDistance;
};
inline RideInfoPresentation rideInfoPresentation(const ClusterState& state,const DisplayOptions& options) {
    RideInfoPresentation result{options.track?"BEST LAP":tripTitle(options.tripPage),ValueText{},options.imperial?"mi":"km","—",ValueText{},!options.track};
    if(options.track) result.tripValue=formatted("%s",lapText(state.bestLapMs).value);
    else {
        const auto distance=tripReading(state,options.tripPage);
        if(distance.available()) result.tripValue=formatted("%.1f",distance.value*(options.imperial?0.621371:1));
    }
    switch(options.infoPage) {
    case InfoPage::Coolant:
        result.infoTitle="COOLANT";
        result.infoValue=state.coolantTemperatureC.available()?formatted("%.0f°C",state.coolantTemperatureC.value):formatted("—°C");
        break;
    case InfoPage::Battery:
        result.infoTitle="BATTERY VOLTAGE";
        result.infoValue=state.batteryVoltageV.available()?formatted("%.1f V",state.batteryVoltageV.value):formatted("— V");
        break;
    case InfoPage::Tcs:
        result.infoTitle="TCS";
        result.infoValue=formatted("%s",state.tcsEnabled.available()?(state.tcsEnabled.value?"ON":"OFF"):"—");
        break;
    case InfoPage::Track:
        result.infoTitle="TRACK MODE"; result.infoValue=formatted("%s",options.track?"ON":"OFF"); break;
    case InfoPage::Count:break;
    }
    return result;
}
enum class LampTone { Active, Muted, Unknown };
struct LampPresentation { LampTone tone; unsigned char opacity; };
inline LampPresentation lampPresentation(const Signal<bool>& signal,bool phase,bool iconTest) {
    if(iconTest) return {LampTone::Active,255};
    if(signal.available()) return signal.value&&phase?LampPresentation{LampTone::Active,255}:LampPresentation{LampTone::Muted,31};
    return signal.quality==Quality::Unsupported?LampPresentation{LampTone::Muted,31}:LampPresentation{LampTone::Unknown,150};
}
} // namespace cluster::presentation
