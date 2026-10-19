#pragma once
#include "DisplayOptions.h"
#include "Lifecycle.h"
namespace cluster {
enum class SettingsAction { Open, Back, Next, Previous, Left, Right, Up, Down };
// User-selected UI access threshold, not a verified vehicle safety limit.
inline constexpr int settingsSpeedLimitKph=20;
inline bool canOpenSettings(const ClusterState& state,const Lifecycle& life) {
    return life.phase()==Phase::Running && state.speedKph.available() && state.speedKph.value>=0 && state.speedKph.value<settingsSpeedLimitKph && !urgent(state);
}
inline void settingsAction(DisplayOptions& options,SettingsAction action,const ClusterState& state,const Lifecycle& life) {
    if(action==SettingsAction::Back) { options.settings=false; return; }
    if(action==SettingsAction::Open) { if(canOpenSettings(state,life)) options.settings=true; return; }
    if(!options.settings) return;
    switch(action) {
    case SettingsAction::Next: case SettingsAction::Right: case SettingsAction::Down: options.settingsFocus=(options.settingsFocus+1)%4; break;
    case SettingsAction::Previous: case SettingsAction::Left: case SettingsAction::Up: options.settingsFocus=(options.settingsFocus+3)%4; break;
    default:break;
    }
}
inline void reconcileSettings(DisplayOptions& options,const ClusterState& state,const Lifecycle& life) {
    if(!canOpenSettings(state,life)) options.settings=false;
}
}
