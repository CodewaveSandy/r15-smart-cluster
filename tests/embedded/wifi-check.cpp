#include "simulator/lvgl/WifiDemo.h"
#include <cstdio>
int main() {
    using namespace cluster;
    int failures=0;
    const auto check=[&](bool ok,const char* why){if(!ok){++failures;std::fprintf(stderr,"FAIL: %s\n",why);}};
    ClusterState s;s.speedKph={0,Quality::Available};
    Lifecycle life;life.key(true,0);life.advance(Lifecycle::bootMs);
    DisplayOptions o;o.settings=true;o.settingsFocus=2;
    o.wifi=true; // Legacy adapter checks; no separate Wi-Fi page in rider navigation.
    check(wifiMenu(s.wifi,false).items[0]==WifiItem::Back,"Unavailable offers no commands");
    host::WifiDemo::populate(s.wifi);host::WifiDemo demo;
    auto e=settingsAction(o,SettingsAction::Select,s,life);
    check(e==SettingsEffect::WifiConnect&&!s.wifi.connected,"Request does not invent connection");
    demo.request(e,s,o.wifiNetwork,100);check(s.wifi.phase==WifiPhase::Connecting,"Connecting observation");
    demo.request(SettingsEffect::WifiCancel,s,0,200);demo.update(s,2000);check(!s.wifi.connected,"Cancel prevents late connection");
    demo.request(e,s,0,2100);demo.update(s,3100);check(s.wifi.connected&&s.wifi.networks[0].saved,"Connection completion");
    o.wifiFocus=1;settingsAction(o,SettingsAction::Select,s,life);check(o.wifiForgetConfirmation,"Forget confirmation");
    check(settingsAction(o,SettingsAction::Select,s,life)==SettingsEffect::None&&!o.wifiForgetConfirmation,"Cancel is confirmation default");
    demo.failNext();demo.request(SettingsEffect::WifiDisconnect,s,0,3200);demo.update(s,4200);
    check(s.wifi.connected&&s.wifi.phase==WifiPhase::Failed,"Failure preserves confirmed connection");
    demo.request(SettingsEffect::WifiForget,s,0,4300);demo.update(s,5300);check(!s.wifi.connected&&!s.wifi.networks[0].saved,"Forget clears saved demo network");
    o.wifiFocus=1;s.wifi.phase=WifiPhase::Idle;
    check(settingsAction(o,SettingsAction::Select,s,life)==SettingsEffect::None&&o.wifiCredentialNotice,"Protected unsaved network has explicit pending credentials step");
    s.speedKph.value=20;check(reconcileSettings(o,s,life)==SettingsEffect::WifiCancel&&!o.wifi&&!o.settings&&!o.wifiCredentialNotice,"Threshold cancels Wi-Fi UI");
    s.wifi.count=4;check(!valid(s.wifi),"Reject oversized list");s.wifi.count=3;
    s.wifi.networks[0].name.fill('X');check(!valid(s.wifi),"Reject unterminated SSID");
    s.wifi.quality=Quality::Stale;check(valid(s.wifi)&&wifiMenu(s.wifi,false).count==1,"Ignore stale payload and disable operations");
    return failures?1:0;
}
