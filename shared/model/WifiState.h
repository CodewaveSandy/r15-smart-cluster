#pragma once
#include "Quality.h"
#include <array>
#include <cstdint>
namespace cluster {
enum class WifiPhase : std::uint8_t { Idle, Scanning, Connecting, Disconnecting, Forgetting, Failed };
struct WifiNetwork { std::array<char,33> name{}; bool secured=true, saved=false; };
struct WifiState {
    Quality quality=Quality::Unavailable;
    WifiPhase phase=WifiPhase::Idle;
    bool connected=false;
    std::array<char,33> networkName{};
    std::array<WifiNetwork,3> networks{};
    unsigned count=0;
    bool available() const { return quality==Quality::Available; }
};
inline bool valid(const WifiState& s) {
    if(unsigned(s.quality)>unsigned(Quality::Stale)) return false;
    if(!s.available()) return true;
    if(unsigned(s.phase)>unsigned(WifiPhase::Failed)||s.count>s.networks.size()) return false;
    const auto terminated=[](const auto& a){for(char c:a) if(!c) return true;return false;};
    if(!terminated(s.networkName)) return false;
    for(unsigned i=0;i<s.count;++i) if(!terminated(s.networks[i].name)) return false;
    return true;
}
}
