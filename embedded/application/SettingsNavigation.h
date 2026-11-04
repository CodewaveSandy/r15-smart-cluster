#pragma once
#include "DisplayOptions.h"
#include "Lifecycle.h"
#include "BluetoothMenu.h"
#include "WifiMenu.h"
#include "PhoneMenu.h"
namespace cluster {
enum class SettingsAction { Open, Back, Next, Previous, Left, Right, Up, Down, Select };
enum class SettingsEffect { PhoneApprove, PhoneReject, PhonePair, PhoneDisconnect, PhoneForget, None, SavePreferences, SaveClockPreferences, BluetoothPair, BluetoothConnect, BluetoothDisconnect, BluetoothForget, BluetoothCancel, WifiScan, WifiConnect, WifiDisconnect, WifiForget, WifiCancel };
inline SettingsEffect closeWifi(DisplayOptions& o) { const bool open=o.wifi;o.wifi=false;o.wifiForgetConfirmation=false;o.wifiCredentialNotice=false;return open?SettingsEffect::WifiCancel:SettingsEffect::None; }
inline SettingsEffect closeBluetooth(DisplayOptions& options) {
    const bool wasOpen=options.bluetooth;
    options.bluetooth=false; options.bluetoothForgetConfirmation=false;
    return wasOpen?SettingsEffect::BluetoothCancel:SettingsEffect::None;
}
inline SettingsEffect closeRadio(DisplayOptions& o) { if(o.wifi) return closeWifi(o);return closeBluetooth(o); }
inline void cancelAppearance(DisplayOptions& options) {
    if(options.appearance) options.light=options.appearanceOriginalLight;
    options.appearance=false; options.appearanceSaveFailed=false; options.appearanceSaving=false;
}
inline void appearanceSaveResult(DisplayOptions& options,bool saved) {
    options.appearanceSaving=false;
    options.appearanceSaveFailed=!saved;
    if(saved) { options.appearanceOriginalLight=options.light; options.appearance=false; }
}
inline void cancelClock(DisplayOptions& options) {
    if(options.clock) options.clockPreferences=options.clockOriginal;
    options.clock=false;options.clockSaving=false;options.clockSaveFailed=false;
}
inline void clockSaveResult(DisplayOptions& options,bool saved) {
    options.clockSaving=false;options.clockSaveFailed=!saved;
    if(saved){options.clockOriginal=options.clockPreferences;options.clock=false;}
}
// Prototype access is unrestricted by telemetry/warnings at the user's request.
// Lifecycle still controls whether an application screen is visible/powered.
inline bool canOpenSettings(const ClusterState&,const Lifecycle& life) {return life.phase()==Phase::Running;}
inline SettingsEffect settingsAction(DisplayOptions& options,SettingsAction action,const ClusterState& state,const Lifecycle& life) {
    if(action==SettingsAction::Back) {
        if(options.map){if(options.mapPan)options.mapPan=false;else options.map=false;return SettingsEffect::None;}
        if(options.clock){cancelClock(options);return SettingsEffect::None;}
        if(options.phone){
            if(options.phoneForgetConfirmation){options.phoneForgetConfirmation=false;options.phoneFocus=0;}
            else {options.phone=false;if(state.phone.pairingRequested&&!state.phone.pairingApproved)return SettingsEffect::PhoneReject;}
            return SettingsEffect::None;
        }
        if(options.appearance) cancelAppearance(options);
        else if(options.wifi) {
            if(options.wifiForgetConfirmation||options.wifiCredentialNotice) {options.wifiForgetConfirmation=false;options.wifiCredentialNotice=false;options.wifiFocus=0;return SettingsEffect::None;}
            return closeWifi(options);
        }
        else if(options.bluetooth) {
            if(options.bluetoothForgetConfirmation) { options.bluetoothForgetConfirmation=false; options.bluetoothFocus=0; return SettingsEffect::None; }
            return closeBluetooth(options);
        }
        else options.settings=false;
        return SettingsEffect::None;
    }
    if(action==SettingsAction::Open) { if(canOpenSettings(state,life)) options.settings=true; return SettingsEffect::None; }
    if(!options.settings) return SettingsEffect::None;
    if(!canOpenSettings(state,life)) { options.phone=false;options.phoneForgetConfirmation=false;cancelAppearance(options); cancelClock(options); options.settings=false; options.map=false; const auto effect=closeRadio(options);return state.phone.pairingRequested?SettingsEffect::PhoneReject:effect; }
    if(options.map){
        if(options.mapPan){
            if(action==SettingsAction::Next)options.mapPanFocus=(options.mapPanFocus+1)%5;
            else if(action==SettingsAction::Previous)options.mapPanFocus=(options.mapPanFocus+4)%5;
            else if(action==SettingsAction::Select){
                if(options.mapPanFocus==4)options.mapPan=false;
                else {const int dx[]={96,-96,0,0},dy[]={0,0,72,-72};options.mapCamera.drag(dx[options.mapPanFocus],dy[options.mapPanFocus]);}
            }else if(action==SettingsAction::Left)options.mapCamera.drag(96,0);
            else if(action==SettingsAction::Right)options.mapCamera.drag(-96,0);
            else if(action==SettingsAction::Up)options.mapCamera.drag(0,72);
            else if(action==SettingsAction::Down)options.mapCamera.drag(0,-72);
            return SettingsEffect::None;
        }
        if(action==SettingsAction::Next||action==SettingsAction::Right||action==SettingsAction::Down)options.mapFocus=(options.mapFocus+1)%6;
        else if(action==SettingsAction::Previous||action==SettingsAction::Left||action==SettingsAction::Up)options.mapFocus=(options.mapFocus+5)%6;
        else if(action==SettingsAction::Select){
            if(options.mapFocus==0)options.map=false;
            else if(options.mapFocus==1){options.mapCamera.follow();}
            else if(options.mapFocus==2){options.mapCamera.area();}
            else if(options.mapFocus==3){options.mapCamera.zoom();if(options.mapZoom>1)--options.mapZoom;}
            else if(options.mapFocus==4){options.mapCamera.zoom();if(options.mapZoom<options.mapCamera.zoomLimit())++options.mapZoom;}
            else if(options.mapFocus==5){options.mapPan=true;options.mapPanFocus=0;options.mapCamera.browse();}
        }
    } else if(options.clock) {
        if(options.clockSaving)return SettingsEffect::None;
        if(action==SettingsAction::Next||action==SettingsAction::Right||action==SettingsAction::Down)options.clockFocus=(options.clockFocus+1)%7;
        else if(action==SettingsAction::Previous||action==SettingsAction::Left||action==SettingsAction::Up)options.clockFocus=(options.clockFocus+6)%7;
        else if(action==SettingsAction::Select) {
            auto& prefs=options.clockPreferences;
            switch(options.clockFocus) {
            case 0:prefs.twelveHour=!prefs.twelveHour;break;
            case 1:prefs.dateFormat=DateFormat((unsigned(prefs.dateFormat)+1)%3);break;
            case 2:if(prefs.automaticTimezone)prefs.utcOffsetMinutes=std::int16_t(options.clockSourceOffset);prefs.automaticTimezone=!prefs.automaticTimezone;break;
            case 3:if(prefs.automaticTimezone)prefs.utcOffsetMinutes=std::int16_t(options.clockSourceOffset);prefs.automaticTimezone=false;prefs.utcOffsetMinutes=std::int16_t(prefs.utcOffsetMinutes<=-720?-720:prefs.utcOffsetMinutes-15);break;
            case 4:if(prefs.automaticTimezone)prefs.utcOffsetMinutes=std::int16_t(options.clockSourceOffset);prefs.automaticTimezone=false;prefs.utcOffsetMinutes=std::int16_t(prefs.utcOffsetMinutes>=840?840:prefs.utcOffsetMinutes+15);break;
            case 5:cancelClock(options);break;
            case 6:return SettingsEffect::SaveClockPreferences;
            }
        }
    } else if(options.phone){
        const auto menu=phoneMenu(state.phone,options.phoneForgetConfirmation);
        options.phoneFocus%=menu.count;
        if(action==SettingsAction::Next||action==SettingsAction::Right||action==SettingsAction::Down)options.phoneFocus=(options.phoneFocus+1)%menu.count;
        else if(action==SettingsAction::Previous||action==SettingsAction::Left||action==SettingsAction::Up)options.phoneFocus=(options.phoneFocus+menu.count-1)%menu.count;
        else if(action==SettingsAction::Select){switch(menu.items[options.phoneFocus]){
            case PhoneItem::Approve:return SettingsEffect::PhoneApprove;
            case PhoneItem::Reject:return SettingsEffect::PhoneReject;
            case PhoneItem::Pair:return SettingsEffect::PhonePair;
            case PhoneItem::Disconnect:return SettingsEffect::PhoneDisconnect;
            case PhoneItem::Forget:options.phoneForgetConfirmation=true;options.phoneFocus=0;break;
            case PhoneItem::ConfirmForget:options.phoneForgetConfirmation=false;options.phoneFocus=0;return SettingsEffect::PhoneForget;
            case PhoneItem::Cancel:options.phoneForgetConfirmation=false;options.phoneFocus=0;break;
            case PhoneItem::Back:options.phone=false;break;
        }}
    } else if(options.wifi) {
        if(!state.wifi.available()||!valid(state.wifi)||options.wifiNetwork>=state.wifi.count||state.wifi.phase!=WifiPhase::Idle) options.wifiCredentialNotice=false;
        const auto menu=wifiMenu(state.wifi,options.wifiForgetConfirmation);
        options.wifiFocus%=options.wifiCredentialNotice?1u:menu.count;
        const auto count=options.wifiCredentialNotice?1u:menu.count;
        if(action==SettingsAction::Next||action==SettingsAction::Right||action==SettingsAction::Down) options.wifiFocus=(options.wifiFocus+1)%count;
        else if(action==SettingsAction::Previous||action==SettingsAction::Left||action==SettingsAction::Up) options.wifiFocus=(options.wifiFocus+count-1)%count;
        else if(action==SettingsAction::Select) {
            if(options.wifiCredentialNotice) {options.wifiCredentialNotice=false;return SettingsEffect::WifiConnect;}
            switch(menu.items[options.wifiFocus]) {
            case WifiItem::Network:
                options.wifiNetwork=options.wifiFocus;
                if(state.wifi.networks[options.wifiNetwork].secured&&!state.wifi.networks[options.wifiNetwork].saved) {options.wifiCredentialNotice=true;break;}
                return SettingsEffect::WifiConnect;
            case WifiItem::Scan:return SettingsEffect::WifiScan;
            case WifiItem::Disconnect:return SettingsEffect::WifiDisconnect;
            case WifiItem::Forget:options.wifiForgetConfirmation=true;options.wifiFocus=0;break;
            case WifiItem::ConfirmForget:options.wifiForgetConfirmation=false;options.wifiFocus=0;return SettingsEffect::WifiForget;
            case WifiItem::Cancel:
                if(options.wifiForgetConfirmation) {options.wifiForgetConfirmation=false;options.wifiFocus=0;break;}
                return SettingsEffect::WifiCancel;
            case WifiItem::Back:return closeWifi(options);
            }
        }
    } else if(options.bluetooth) {
        const auto menu=bluetoothMenu(state.bluetooth,options.bluetoothForgetConfirmation);
        options.bluetoothFocus%=menu.count;
        switch(action) {
        case SettingsAction::Next:case SettingsAction::Right:case SettingsAction::Down:
            options.bluetoothFocus=(options.bluetoothFocus+1)%menu.count; break;
        case SettingsAction::Previous:case SettingsAction::Left:case SettingsAction::Up:
            options.bluetoothFocus=(options.bluetoothFocus+menu.count-1)%menu.count; break;
        case SettingsAction::Select:
            switch(menu.items[options.bluetoothFocus]) {
            case BluetoothItem::Pair:options.bluetoothFocus=0;return SettingsEffect::BluetoothPair;
            case BluetoothItem::Connect:options.bluetoothFocus=0;return SettingsEffect::BluetoothConnect;
            case BluetoothItem::Disconnect:options.bluetoothFocus=0;return SettingsEffect::BluetoothDisconnect;
            case BluetoothItem::Forget:options.bluetoothForgetConfirmation=true;options.bluetoothFocus=0;break;
            case BluetoothItem::CancelForget:options.bluetoothForgetConfirmation=false;options.bluetoothFocus=0;break;
            case BluetoothItem::ConfirmForget:options.bluetoothForgetConfirmation=false;options.bluetoothFocus=0;return SettingsEffect::BluetoothForget;
            case BluetoothItem::CancelOperation:options.bluetoothFocus=0;return SettingsEffect::BluetoothCancel;
            case BluetoothItem::Back:return closeBluetooth(options);
            }
            break;
        default:break;
        }
    } else if(options.appearance) {
        if(options.appearanceSaving) return SettingsEffect::None;
        switch(action) {
        case SettingsAction::Next:case SettingsAction::Right:case SettingsAction::Down:
            options.appearanceFocus=(options.appearanceFocus+1)%5; break;
        case SettingsAction::Previous:case SettingsAction::Left:case SettingsAction::Up:
            options.appearanceFocus=(options.appearanceFocus+4)%5; break;
        case SettingsAction::Select:
            if(options.appearanceFocus<2) options.light=options.appearanceFocus==1;
            else if(options.appearanceFocus==3) cancelAppearance(options);
            else if(options.appearanceFocus==4) return SettingsEffect::SavePreferences;
            break;
        default:break;
        }
    } else {
        switch(action) {
        case SettingsAction::Next: case SettingsAction::Right: case SettingsAction::Down: options.settingsFocus=(options.settingsFocus+1)%4; break;
        case SettingsAction::Previous: case SettingsAction::Left: case SettingsAction::Up: options.settingsFocus=(options.settingsFocus+3)%4; break;
        case SettingsAction::Select:
            if(options.settingsFocus==0) {
                options.appearance=true; options.appearanceOriginalLight=options.light;
                options.appearanceFocus=options.light?1:0; options.appearanceSaveFailed=false; options.appearanceSaving=false;
            } else if(options.settingsFocus==1) {
                options.phone=true;options.phoneFocus=0;options.phoneForgetConfirmation=false;
            } else if(options.settingsFocus==2){options.map=true;options.mapPan=false;options.mapFocus=1;} else if(options.settingsFocus==3) {
                options.clock=true;options.clockOriginal=options.clockPreferences;options.clockFocus=0;
                options.clockSaveFailed=false;options.clockSaving=false;
            }
            break;
        default:break;
        }
    }
    return SettingsEffect::None;
}
inline SettingsEffect reconcileSettings(DisplayOptions& options,const ClusterState& state,const Lifecycle& life) {
    if(!canOpenSettings(state,life)) {
        options.phone=false;options.phoneForgetConfirmation=false;cancelAppearance(options); cancelClock(options); options.settings=false; options.map=false; const auto effect=closeRadio(options);return state.phone.pairingRequested?SettingsEffect::PhoneReject:effect;
    }
    if(options.phone){
        if(!state.phone.available()||!state.phone.paired)options.phoneForgetConfirmation=false;
        options.phoneFocus%=phoneMenu(state.phone,options.phoneForgetConfirmation).count;
    }
    if(options.wifi) {
        if(!state.wifi.available()||!valid(state.wifi)||state.wifi.phase!=WifiPhase::Idle) {options.wifiForgetConfirmation=false;options.wifiCredentialNotice=false;}
        if(!state.wifi.connected) options.wifiForgetConfirmation=false;
        if(options.wifiNetwork>=state.wifi.count) options.wifiCredentialNotice=false;
        options.wifiFocus%=wifiMenu(state.wifi,options.wifiForgetConfirmation).count;
    }
    if(options.bluetooth) {
        if(!state.bluetooth.available()||!state.bluetooth.paired||
                (state.bluetooth.phase!=BluetoothPhase::Idle&&state.bluetooth.phase!=BluetoothPhase::Failed)) options.bluetoothForgetConfirmation=false;
        options.bluetoothFocus%=bluetoothMenu(state.bluetooth,options.bluetoothForgetConfirmation).count;
    }
    return SettingsEffect::None;
}
}
