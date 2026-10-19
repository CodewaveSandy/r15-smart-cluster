#pragma once
#include <array>
#include <cstdint>
#include "Quality.h"
// Normalized radio observations; no LVGL, RTOS, SDK or native wire encoding.
namespace cluster {
enum class BluetoothPhase : std::uint8_t { Idle, Discoverable, Pairing, Connecting, Disconnecting, Forgetting, Failed };
enum class BluetoothFailure : std::uint8_t { None, Timeout, Rejected, Unavailable };
struct BluetoothState {
    Quality quality=Quality::Unavailable;
    BluetoothPhase phase=BluetoothPhase::Idle;
    BluetoothFailure failure=BluetoothFailure::None;
    bool paired=false, connected=false;
    std::array<char,32> deviceName{};
    bool available() const { return quality==Quality::Available; }
};
inline bool valid(const BluetoothState& state) {
    if(unsigned(state.quality)>unsigned(Quality::Stale)) return false;
    if(!state.available()) return true;
    if(unsigned(state.phase)>unsigned(BluetoothPhase::Failed)||unsigned(state.failure)>unsigned(BluetoothFailure::Unavailable)) return false;
    for(char c:state.deviceName) if(c=='\0') return true;
    return false;
}
}
