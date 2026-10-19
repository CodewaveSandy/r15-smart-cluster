#pragma once
#include "embedded/application/SettingsNavigation.h"
#include <cstdio>
namespace cluster::host {
class WifiDemo {
public:
    static void populate(WifiState& s) {
        s.quality=Quality::Available;s.count=3;
        const char* names[]={"Rider hotspot","Garage Wi-Fi","Guest network"};
        for(unsigned i=0;i<3;++i) {std::snprintf(s.networks[i].name.data(),33,"%s",names[i]);s.networks[i].secured=i!=2;}
        s.networks[0].saved=true;
    }
    void failNext(){failNext_=true;}
    void toggleAvailability(ClusterState& s) {pending_=SettingsEffect::None;s.wifi.quality=s.wifi.available()?Quality::Unavailable:Quality::Available;s.wifi.phase=WifiPhase::Idle;}
    void request(SettingsEffect e,ClusterState& s,unsigned index,std::uint64_t now) {
        auto& w=s.wifi;
        if(e==SettingsEffect::WifiCancel) {
            if(pending_==SettingsEffect::WifiScan||pending_==SettingsEffect::WifiConnect) {pending_=SettingsEffect::None;w.phase=WifiPhase::Idle;}return;
        }
        if(!w.available()||pending_!=SettingsEffect::None) return;
        switch(e) {
        case SettingsEffect::WifiScan:w.phase=WifiPhase::Scanning;break;
        case SettingsEffect::WifiConnect:if(index>=w.count) return;w.phase=WifiPhase::Connecting;index_=index;break;
        case SettingsEffect::WifiDisconnect:if(!w.connected)return;w.phase=WifiPhase::Disconnecting;break;
        case SettingsEffect::WifiForget:if(!w.connected)return;w.phase=WifiPhase::Forgetting;break;
        default:return;
        }
        pending_=e;entered_=now;failure_=failNext_;failNext_=false;
    }
    void update(ClusterState& s,std::uint64_t now) {
        auto& w=s.wifi;if(pending_==SettingsEffect::None||now<entered_||now-entered_<1000)return;
        if(failure_) w.phase=WifiPhase::Failed;
        else {
            if(pending_==SettingsEffect::WifiScan&&w.count==0) populate(w);
            // Existing fixture scans preserve saved-network choices.
            if(pending_==SettingsEffect::WifiConnect) {w.connected=true;w.networkName=w.networks[index_].name;w.networks[index_].saved=true;}
            if(pending_==SettingsEffect::WifiForget) for(auto& n:w.networks) if(n.name==w.networkName)n.saved=false;
            if(pending_==SettingsEffect::WifiDisconnect||pending_==SettingsEffect::WifiForget) {w.connected=false;w.networkName.fill(0);}
            w.phase=WifiPhase::Idle;
        }
        pending_=SettingsEffect::None;
    }
private:
    SettingsEffect pending_=SettingsEffect::None;
    unsigned index_=0;std::uint64_t entered_=0;
    bool failNext_=false,failure_=false;
};
}
