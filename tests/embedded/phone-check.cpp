#include "embedded/application/SettingsNavigation.h"
#include <cassert>
#include <cstring>
int main(){
    using namespace cluster;
    ClusterState state;state.speedKph={0,Quality::Available};state.phone.quality=Quality::Available;
    std::strcpy(state.phone.pairingCode.data(),"123456");
    assert(valid(state));Lifecycle life;life.key(true,0);life.advance(3000);
    DisplayOptions options;options.settings=true;options.settingsFocus=1;
    settingsAction(options,SettingsAction::Select,state,life);assert(options.phone&&!options.bluetooth&&!options.wifi);
    assert(settingsAction(options,SettingsAction::Select,state,life)==SettingsEffect::PhonePair);
    state.speedKph.value=1;assert(settingsAction(options,SettingsAction::Select,state,life)==SettingsEffect::None);
    state.speedKph.value=0;state.phone.paired=true;state.phone.connected=true;
    assert(settingsAction(options,SettingsAction::Select,state,life)==SettingsEffect::PhoneDisconnect);
    options.phoneFocus=1;settingsAction(options,SettingsAction::Select,state,life);assert(options.phoneForgetConfirmation&&options.phoneFocus==0);
    settingsAction(options,SettingsAction::Select,state,life);assert(!options.phoneForgetConfirmation);
    options.phoneFocus=1;settingsAction(options,SettingsAction::Select,state,life);settingsAction(options,SettingsAction::Next,state,life);
    assert(settingsAction(options,SettingsAction::Select,state,life)==SettingsEffect::PhoneForget);
    state.phone.paired=false;state.phone.connected=false;state.phone.pairingRequested=true;
    options.phoneFocus=0;assert(settingsAction(options,SettingsAction::Select,state,life)==SettingsEffect::PhoneReject);
    settingsAction(options,SettingsAction::Next,state,life);
    assert(settingsAction(options,SettingsAction::Select,state,life)==SettingsEffect::PhoneApprove);
    state.speedKph.value=1;assert(settingsAction(options,SettingsAction::Select,state,life)==SettingsEffect::None);state.speedKph.value=0;
    state.phone.pairingRequested=false;state.phone.paired=true;state.phone.connected=true;
    state.phone.quality=Quality::Unavailable;assert(phoneMenu(state.phone,true).count==1);
    state.engineWarning={true,Quality::Available};reconcileSettings(options,state,life);assert(!options.settings&&!options.phone);
    state.phone.quality=Quality::Available;state.phone.paired=false;assert(!valid(state.phone));
}
