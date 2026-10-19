#pragma once
#include "shared/model/WifiState.h"
namespace cluster {
enum class WifiItem { Network, Scan, Disconnect, Forget, Cancel, ConfirmForget, Back };
struct WifiMenu { WifiItem items[5]{}; unsigned count=0; };
inline WifiMenu wifiMenu(const WifiState& s,bool confirmation) {
    WifiMenu m;
    if(!s.available()||!valid(s)) { m.items[m.count++]=WifiItem::Back;return m; }
    if(confirmation&&s.connected) {m.items[m.count++]=WifiItem::Cancel;m.items[m.count++]=WifiItem::ConfirmForget;return m;}
    if(s.phase!=WifiPhase::Idle&&s.phase!=WifiPhase::Failed) {m.items[m.count++]=WifiItem::Cancel;return m;}
    if(s.connected) {m.items[m.count++]=WifiItem::Disconnect;m.items[m.count++]=WifiItem::Forget;}
    else for(unsigned i=0;i<s.count;++i) m.items[m.count++]=WifiItem::Network;
    m.items[m.count++]=WifiItem::Scan;
    return m;
}
inline const char* wifiStatus(const WifiState& s) {
    if(!s.available()||!valid(s)) return "Status unavailable";
    switch(s.phase) {
    case WifiPhase::Scanning:return "Searching for networks...";
    case WifiPhase::Connecting:return "Connecting...";
    case WifiPhase::Disconnecting:return "Disconnecting...";
    case WifiPhase::Forgetting:return "Forgetting network...";
    case WifiPhase::Failed:return s.connected?"Connected - last request failed":"Connection failed - select to retry";
    default:return s.connected?"Connected":"Choose a network or search again";
    }
}
// SSIDs are arbitrary bytes. Keep unsupported text out of the bundled font subset.
inline const char* wifiName(const std::array<char,33>& name) {
    unsigned n=0;for(char c:name) {if(!c) return n?name.data():"Unnamed network";if(c<32||c>126||++n>24) return "Network name unavailable";}
    return "Network name unavailable";
}
}
