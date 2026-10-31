#pragma once
#include "Quality.h"
#include <array>
#include <cstdint>
namespace cluster {
enum class PhoneUnavailable {Service, Status, VehicleSession, ReturnChannel};
struct PhoneState {
    PhoneUnavailable unavailable=PhoneUnavailable::Service;
    Quality quality=Quality::Unavailable;
    std::array<char,33> displayName{}; // Optional untrusted current-connection label.
    std::uint32_t connectionId=0;
    int batteryPercent=-1;
    bool bleBench=false,pairWindowOpen=false,commandFailed=false;
    bool forgetting=false; // Display-local pending command; never serialized.
    bool paired=false,connected=false,pairingRequested=false,pairingApproved=false;
    std::array<char,7> pairingCode{};
    bool available()const{return quality==Quality::Available;}
};
inline bool valid(const PhoneState& state){
    if(unsigned(state.quality)>unsigned(Quality::Stale))return false;
    if(!state.available())return true;
    if(state.paired&&state.pairWindowOpen)return false;
    if(state.batteryPercent< -1||state.batteryPercent>100)return false;
    if(state.displayName.back())return false;
    bool ended=false;for(char c:state.displayName){if(!c){ended=true;continue;}if(ended||c<32||c>126)return false;}
    if(state.connected&&!state.paired)return false;
    if(state.paired&&state.pairingRequested)return false;
    if(state.pairingApproved&&!state.pairingRequested)return false;
    if(state.pairingCode[6]!='\0')return false;
    for(unsigned i=0;i<6;++i){if(state.pairingCode[0]=='\0'){if(state.pairingCode[i]!='\0')return false;}else if(state.pairingCode[i]<'0'||state.pairingCode[i]>'9')return false;}
    return true;
}
}
