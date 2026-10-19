#pragma once
#include "DisplayOptions.h"
#include "Lifecycle.h"
#include "BluetoothMenu.h"
namespace cluster {
enum class SettingsAction { Open, Back, Next, Previous, Left, Right, Up, Down, Select };
enum class SettingsEffect { None, SavePreferences, BluetoothPair, BluetoothConnect, BluetoothDisconnect, BluetoothForget, BluetoothCancel };
inline SettingsEffect closeBluetooth(DisplayOptions& options) {
    const bool wasOpen=options.bluetooth;
    options.bluetooth=false; options.bluetoothForgetConfirmation=false;
    return wasOpen?SettingsEffect::BluetoothCancel:SettingsEffect::None;
}
inline void cancelAppearance(DisplayOptions& options) {
    if(options.appearance) options.light=options.appearanceOriginalLight;
    options.appearance=false; options.appearanceSaveFailed=false;
}
inline void appearanceSaveResult(DisplayOptions& options,bool saved) {
    options.appearanceSaveFailed=!saved;
    if(saved) { options.appearanceOriginalLight=options.light; options.appearance=false; }
}
// User-selected UI access threshold, not a verified vehicle safety limit.
inline constexpr int settingsSpeedLimitKph=20;
inline bool canOpenSettings(const ClusterState& state,const Lifecycle& life) {
    return life.phase()==Phase::Running && state.speedKph.available() && state.speedKph.value>=0 && state.speedKph.value<settingsSpeedLimitKph && !urgent(state);
}
inline SettingsEffect settingsAction(DisplayOptions& options,SettingsAction action,const ClusterState& state,const Lifecycle& life) {
    if(action==SettingsAction::Back) {
        if(options.appearance) cancelAppearance(options);
        else if(options.bluetooth) {
            if(options.bluetoothForgetConfirmation) { options.bluetoothForgetConfirmation=false; options.bluetoothFocus=0; return SettingsEffect::None; }
            return closeBluetooth(options);
        }
        else options.settings=false;
        return SettingsEffect::None;
    }
    if(action==SettingsAction::Open) { if(canOpenSettings(state,life)) options.settings=true; return SettingsEffect::None; }
    if(!options.settings) return SettingsEffect::None;
    if(!canOpenSettings(state,life)) { cancelAppearance(options); options.settings=false; return closeBluetooth(options); }
    if(options.bluetooth) {
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
                options.appearanceFocus=options.light?1:0; options.appearanceSaveFailed=false;
            } else if(options.settingsFocus==1) {
                options.bluetooth=true; options.bluetoothFocus=0; options.bluetoothForgetConfirmation=false;
            }
            break;
        default:break;
        }
    }
    return SettingsEffect::None;
}
inline SettingsEffect reconcileSettings(DisplayOptions& options,const ClusterState& state,const Lifecycle& life) {
    if(!canOpenSettings(state,life)) {
        cancelAppearance(options); options.settings=false; return closeBluetooth(options);
    }
    if(options.bluetooth) {
        if(!state.bluetooth.available()||!state.bluetooth.paired||
                (state.bluetooth.phase!=BluetoothPhase::Idle&&state.bluetooth.phase!=BluetoothPhase::Failed)) options.bluetoothForgetConfirmation=false;
        options.bluetoothFocus%=bluetoothMenu(state.bluetooth,options.bluetoothForgetConfirmation).count;
    }
    return SettingsEffect::None;
}
}
