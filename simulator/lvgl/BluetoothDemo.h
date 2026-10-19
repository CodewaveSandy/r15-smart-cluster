#pragma once
#include "embedded/application/SettingsNavigation.h"
#include <cstdio>
namespace cluster::host {
// Timed observations for the host UI only. No radio stack, pairing keys or UART.
class BluetoothDemo {
public:
    void failNext() { failNext_=true; }
    void toggleAvailability(ClusterState& state) {
        unavailable_=!unavailable_; pending_=SettingsEffect::None;
        state.bluetooth.quality=unavailable_?Quality::Unavailable:Quality::Available;
        state.bluetooth.phase=BluetoothPhase::Idle; state.bluetooth.failure=BluetoothFailure::None;
    }
    void request(SettingsEffect effect,ClusterState& state,std::uint64_t now) {
        auto& status=state.bluetooth;
        if(effect==SettingsEffect::BluetoothCancel) {
            if(pending_==SettingsEffect::BluetoothPair||pending_==SettingsEffect::BluetoothConnect) {
                pending_=SettingsEffect::None; status.phase=BluetoothPhase::Idle; status.failure=BluetoothFailure::None;
                if(!status.paired) status.deviceName.fill(0);
            }
            return; // Cancelling a pending operation never disconnects a live link or undoes accepted Forget.
        }
        if(!status.available()||pending_!=SettingsEffect::None) return;
        switch(effect) {
        case SettingsEffect::BluetoothPair:status.phase=BluetoothPhase::Discoverable;status.connected=false;break;
        case SettingsEffect::BluetoothConnect:if(!status.paired) return;status.phase=BluetoothPhase::Connecting;break;
        case SettingsEffect::BluetoothDisconnect:if(!status.connected) return;status.phase=BluetoothPhase::Disconnecting;break;
        case SettingsEffect::BluetoothForget:if(!status.paired) return;status.phase=BluetoothPhase::Forgetting;break;
        default:return;
        }
        pending_=effect; entered_=now; failPending_=failNext_; failNext_=false;
        status.failure=BluetoothFailure::None;
    }
    void update(ClusterState& state,std::uint64_t now) {
        auto& status=state.bluetooth;
        const auto elapsed=now>=entered_?now-entered_:0;
        if(status.available()&&pending_!=SettingsEffect::None) {
            const auto duration=pending_==SettingsEffect::BluetoothPair?2600u:pending_==SettingsEffect::BluetoothConnect?900u:450u;
            if(elapsed>=duration) {
                if(failPending_) { status.phase=BluetoothPhase::Failed; status.failure=BluetoothFailure::Timeout; }
                else {
                    switch(pending_) {
                    case SettingsEffect::BluetoothPair:case SettingsEffect::BluetoothConnect:
                        status.paired=true;status.connected=true;
                        std::snprintf(status.deviceName.data(),status.deviceName.size(),"Rider phone");break;
                    case SettingsEffect::BluetoothDisconnect:status.connected=false;break;
                    case SettingsEffect::BluetoothForget:status.connected=false;status.paired=false;status.deviceName.fill(0);break;
                    default:break;
                    }
                    status.phase=BluetoothPhase::Idle; status.failure=BluetoothFailure::None;
                }
                pending_=SettingsEffect::None;
            } else if(pending_==SettingsEffect::BluetoothPair&&elapsed>=1200) {
                status.phase=BluetoothPhase::Pairing;
                if(!status.paired) std::snprintf(status.deviceName.data(),status.deviceName.size(),"Rider phone");
            }
        }
        state.phoneConnected={status.connected,status.quality};
    }
private:
    SettingsEffect pending_=SettingsEffect::None;
    std::uint64_t entered_=0;
    bool failNext_=false,failPending_=false,unavailable_=false;
};
}
