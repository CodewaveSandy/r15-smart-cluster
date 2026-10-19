#include "embedded/presentation/BluetoothPresentation.h"
#include "embedded/application/SettingsNavigation.h"
#include "simulator/lvgl/BluetoothDemo.h"
#include <cstdio>
#include <cstring>
int main() {
    using namespace cluster; using namespace cluster::presentation;
    int failed=0;
    const auto check=[&](bool ok,const char* why){if(!ok){++failed;std::fprintf(stderr,"FAIL: %s\n",why);}};
    ClusterState state; state.speedKph={0,Quality::Available};
    Lifecycle life; life.key(true,0); life.advance(Lifecycle::bootMs);
    DisplayOptions options; options.settings=true; options.settingsFocus=1;
    settingsAction(options,SettingsAction::Select,state,life);
    check(options.bluetooth,"Bluetooth category opens");
    check(bluetoothMenu(state.bluetooth,false).items[0]==BluetoothItem::Back,"Unknown status offers no radio operation");
    check(std::strcmp(bluetoothPresentation(state.bluetooth,false).status,"Status unavailable")==0,"Unknown connection is not disconnected");
    state.bluetooth.quality=Quality::Available;
    const auto request=settingsAction(options,SettingsAction::Select,state,life);
    check(request==SettingsEffect::BluetoothPair&&!state.bluetooth.paired&&!state.bluetooth.connected&&state.bluetooth.phase==BluetoothPhase::Idle,"UI requests pairing without optimistic radio state");
    host::BluetoothDemo demo;
    demo.request(request,state,100);
    check(state.bluetooth.phase==BluetoothPhase::Discoverable&&!state.bluetooth.connected,"Discovery waits for observation");
    demo.update(state,1300); check(state.bluetooth.phase==BluetoothPhase::Pairing&&!state.bluetooth.paired,"Pairing progress is not a saved bond");
    demo.request(SettingsEffect::BluetoothCancel,state,1400);
    demo.update(state,10000);
    check(!state.bluetooth.paired&&!state.bluetooth.connected&&state.bluetooth.phase==BluetoothPhase::Idle,"Cancelled pairing cannot complete late");
    demo.request(SettingsEffect::BluetoothPair,state,10100); demo.update(state,12700);
    check(state.bluetooth.connected&&state.bluetooth.paired&&state.phoneConnected.value,"Completed pair publishes connection and saved bond");
    demo.request(SettingsEffect::BluetoothDisconnect,state,13000);
    check(state.bluetooth.connected,"Disconnect waits for completion");
    demo.update(state,13450);
    check(!state.bluetooth.connected&&state.bluetooth.paired,"Disconnect preserves pairing");
    check(bluetoothMenu(state.bluetooth,false).items[0]==BluetoothItem::Connect,"Paired disconnected device offers Connect");
    demo.request(SettingsEffect::BluetoothConnect,state,13500);demo.request(SettingsEffect::BluetoothCancel,state,13600);demo.update(state,14000);
    check(!state.bluetooth.connected&&state.bluetooth.paired,"Cancelled connection cannot complete late or erase pairing");
    demo.request(SettingsEffect::BluetoothConnect,state,14000); demo.update(state,14900);
    check(state.bluetooth.connected,"Reconnect works");
    demo.request(SettingsEffect::BluetoothCancel,state,14901);
    check(state.bluetooth.connected,"Leaving page does not disconnect established link");
    options.bluetoothFocus=2;
    check(settingsAction(options,SettingsAction::Select,state,life)==SettingsEffect::None&&options.bluetoothForgetConfirmation,"Forget first opens confirmation");
    check(options.bluetoothFocus==0&&bluetoothMenu(state.bluetooth,true).items[0]==BluetoothItem::CancelForget,"Confirmation defaults to Cancel");
    settingsAction(options,SettingsAction::Back,state,life);
    check(options.bluetooth&&!options.bluetoothForgetConfirmation&&state.bluetooth.paired,"Back cancels confirmation without unpairing");
    options.bluetoothFocus=2;settingsAction(options,SettingsAction::Select,state,life);
    settingsAction(options,SettingsAction::Next,state,life);
    check(settingsAction(options,SettingsAction::Select,state,life)==SettingsEffect::BluetoothForget,"Explicit confirmation requests Forget");
    demo.request(SettingsEffect::BluetoothForget,state,15000);
    demo.request(SettingsEffect::BluetoothCancel,state,15001);demo.update(state,15450);
    check(!state.bluetooth.paired&&!state.bluetooth.connected&&state.bluetooth.deviceName[0]==0,"Accepted Forget clears bond after completion");
    demo.failNext(); demo.request(SettingsEffect::BluetoothPair,state,16000);demo.update(state,18600);
    check(state.bluetooth.phase==BluetoothPhase::Failed&&state.bluetooth.failure==BluetoothFailure::Timeout&&!state.bluetooth.connected,"Pairing timeout is explicit");
    demo.request(SettingsEffect::BluetoothPair,state,19000);demo.update(state,21600);
    check(state.bluetooth.connected,"Retry after failure succeeds");
    demo.failNext();demo.request(SettingsEffect::BluetoothDisconnect,state,22000);demo.update(state,22450);
    check(state.bluetooth.connected&&state.bluetooth.paired&&bluetoothPresentation(state.bluetooth,false).active,"Failed disconnect preserves confirmed live connection");
    state.speedKph.quality=Quality::Stale;
    check(reconcileSettings(options,state,life)==SettingsEffect::BluetoothCancel&&!options.settings&&!options.bluetooth,"Access interruption closes Bluetooth and requests cancellation");
    for(auto quality:{Quality::Unavailable,Quality::Unsupported,Quality::Invalid,Quality::Stale}) {
        state.bluetooth.quality=quality;
        check(bluetoothMenu(state.bluetooth,true).items[0]==BluetoothItem::Back,"Non-current status cannot Forget or Connect");
    }
    state.bluetooth.quality=static_cast<Quality>(255);check(!valid(state.bluetooth),"Reject invalid radio quality");
    state.bluetooth.quality=Quality::Available;state.bluetooth.phase=static_cast<BluetoothPhase>(255);
    check(!valid(state.bluetooth),"Reject invalid phase");
    state.bluetooth.phase=BluetoothPhase::Idle;state.bluetooth.deviceName.fill('W');
    check(!valid(state.bluetooth),"Reject unterminated name");
    state.bluetooth.deviceName.fill(0);state.bluetooth.deviceName[0]=char(0xff);
    check(std::strcmp(bluetoothPresentation(state.bluetooth,false).name.value,"Device name unavailable")==0,"Unsupported display text has a safe fallback");
    return failed?1:0;
}
