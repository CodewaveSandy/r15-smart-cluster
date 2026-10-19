#pragma once
#include "DashboardPresentation.h"
namespace cluster::presentation {
struct BluetoothPresentation { ValueText name; const char* status; bool active=false,error=false; };
inline BluetoothPresentation bluetoothPresentation(const BluetoothState& state,bool confirmation) {
    if(!state.available()) return {formatted("Bluetooth unavailable"),"Status unavailable",false,true};
    ValueText name=formatted(state.paired||state.connected?"Paired device":"No device paired");
    bool supportedName=true; unsigned length=0;
    while(length<state.deviceName.size()&&state.deviceName[length]) {
        const auto c=static_cast<unsigned char>(state.deviceName[length]);
        if(c<32||c>126) supportedName=false;
        ++length;
    }
    if(length&&length<state.deviceName.size()&&(state.paired||state.connected||state.phase==BluetoothPhase::Pairing||state.phase==BluetoothPhase::Connecting||state.phase==BluetoothPhase::Failed))
        name=supportedName?formatted("%.24s",state.deviceName.data()):formatted("Device name unavailable");
    if(confirmation&&state.paired&&(state.phase==BluetoothPhase::Idle||state.phase==BluetoothPhase::Failed)) return {name,"Forget saved pairing?",false,false};
    switch(state.phase) {
    case BluetoothPhase::Discoverable:return {formatted("Ready to pair"),"Continue pairing on your phone",false,false};
    case BluetoothPhase::Pairing:return {name,"Pairing - follow the phone prompt",false,false};
    case BluetoothPhase::Connecting:return {name,"Connecting...",false,false};
    case BluetoothPhase::Disconnecting:return {name,"Disconnecting...",false,false};
    case BluetoothPhase::Forgetting:return {name,"Forgetting device...",false,false};
    case BluetoothPhase::Failed:
        return {name,state.connected?"Connected - last request failed":state.failure==BluetoothFailure::Timeout?"Request timed out":
            state.failure==BluetoothFailure::Rejected?"Pairing declined":state.failure==BluetoothFailure::Unavailable?"Service unavailable":"Connection request failed",state.connected,true};
    case BluetoothPhase::Idle:break;
    }
    return {name,state.connected?"Connected":state.paired?"Paired - disconnected":"Not connected",state.connected,false};
}
}
