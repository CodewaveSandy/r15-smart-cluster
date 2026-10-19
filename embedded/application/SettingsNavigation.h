#pragma once
#include "DisplayOptions.h"
#include "Lifecycle.h"
namespace cluster {
enum class SettingsAction { Open, Back, Next, Previous, Left, Right, Up, Down, Select };
enum class SettingsEffect { None, SavePreferences };
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
        if(options.appearance) cancelAppearance(options); else options.settings=false;
        return SettingsEffect::None;
    }
    if(action==SettingsAction::Open) { if(canOpenSettings(state,life)) options.settings=true; return SettingsEffect::None; }
    if(!options.settings) return SettingsEffect::None;
    if(!canOpenSettings(state,life)) { cancelAppearance(options); options.settings=false; return SettingsEffect::None; }
    if(options.appearance) {
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
            }
            break;
        default:break;
        }
    }
    return SettingsEffect::None;
}
inline void reconcileSettings(DisplayOptions& options,const ClusterState& state,const Lifecycle& life) {
    if(!canOpenSettings(state,life)) { cancelAppearance(options); options.settings=false; }
}
}
