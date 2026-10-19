#pragma once
#include "shared/model/BluetoothState.h"
namespace cluster {
enum class BluetoothItem { Pair, Connect, Disconnect, Forget, CancelOperation, CancelForget, ConfirmForget, Back };
struct BluetoothMenu {
    std::array<BluetoothItem,3> items{{BluetoothItem::Back,BluetoothItem::Back,BluetoothItem::Back}};
    unsigned count=1;
};
inline BluetoothMenu bluetoothMenu(const BluetoothState& state,bool confirmation) {
    if(!state.available()) return {};
    if(confirmation&&state.paired&&(state.phase==BluetoothPhase::Idle||state.phase==BluetoothPhase::Failed))
        return {{{BluetoothItem::CancelForget,BluetoothItem::ConfirmForget,BluetoothItem::Back}},2};
    switch(state.phase) {
    case BluetoothPhase::Discoverable:case BluetoothPhase::Pairing:case BluetoothPhase::Connecting:
        return {{{BluetoothItem::CancelOperation,BluetoothItem::Back,BluetoothItem::Back}},1};
    case BluetoothPhase::Disconnecting:case BluetoothPhase::Forgetting:return {};
    default:break;
    }
    if(state.connected) {
        if(state.paired) return {{{BluetoothItem::Pair,BluetoothItem::Disconnect,BluetoothItem::Forget}},3};
        return {{{BluetoothItem::Pair,BluetoothItem::Disconnect,BluetoothItem::Back}},2};
    }
    if(state.paired) return {{{BluetoothItem::Connect,BluetoothItem::Pair,BluetoothItem::Forget}},3};
    return {{{BluetoothItem::Pair,BluetoothItem::Back,BluetoothItem::Back}},1};
}
inline const char* bluetoothItemTitle(BluetoothItem item,bool retry=false) {
    switch(item) {
    case BluetoothItem::Pair:return retry?"Retry pairing":"Pair new device";
    case BluetoothItem::Connect:return retry?"Retry connection":"Connect";
    case BluetoothItem::Disconnect:return "Disconnect";
    case BluetoothItem::Forget:case BluetoothItem::ConfirmForget:return "Forget device";
    case BluetoothItem::CancelOperation:return "Cancel pairing / connection";
    case BluetoothItem::CancelForget:return "Cancel";
    case BluetoothItem::Back:return "Back";
    }
    return "Back";
}
}
