#pragma once
#include "Quality.h"
#include <array>
namespace cluster {
struct PhoneState {
    Quality quality=Quality::Unavailable;
    bool bleBench=false,pairWindowOpen=false,commandFailed=false;
    bool paired=false,connected=false,pairingRequested=false,pairingApproved=false;
    std::array<char,7> pairingCode{};
    bool available()const{return quality==Quality::Available;}
};
inline bool valid(const PhoneState& state){
    if(unsigned(state.quality)>unsigned(Quality::Stale))return false;
    if(!state.available())return true;
    if(state.paired&&state.pairWindowOpen)return false;
    if(state.connected&&!state.paired)return false;
    if(state.paired&&state.pairingRequested)return false;
    if(state.pairingApproved&&!state.pairingRequested)return false;
    if(state.pairingCode[6]!='\0')return false;
    for(unsigned i=0;i<6;++i){if(state.pairingCode[0]=='\0'){if(state.pairingCode[i]!='\0')return false;}else if(state.pairingCode[i]<'0'||state.pairingCode[i]>'9')return false;}
    return true;
}
}
