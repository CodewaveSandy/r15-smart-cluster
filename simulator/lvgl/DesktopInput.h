#pragma once
#include "embedded/application/DashboardInput.h"
#include "embedded/application/SettingsNavigation.h"
#include <SDL_keycode.h>
#include <optional>
namespace cluster::host {
inline std::optional<SettingsAction> settingsAction(SDL_Keycode key,bool open) {
    if(key==SDLK_s) return SettingsAction::Open;
    if(!open) return std::nullopt;
    switch(key) {
    case SDLK_ESCAPE:case SDLK_BACKSPACE:return SettingsAction::Back;
    case SDLK_DOWN:return SettingsAction::Down;
    case SDLK_i:return SettingsAction::Next;
    case SDLK_UP:return SettingsAction::Up;
    case SDLK_LEFT:return SettingsAction::Left;
    case SDLK_RIGHT:return SettingsAction::Right;
    default:return std::nullopt;
    }
}
// Keyboard knowledge is confined to the host adapter. Demo telemetry keys stay
// in the simulator; physical inputs can emit the same DashboardAction values.
inline std::optional<DashboardAction> dashboardAction(SDL_Keycode key) {
    switch(key) {
    case SDLK_RETURN:return DashboardAction::NextTrip;
    case SDLK_i:return DashboardAction::NextInfo;
    case SDLK_t:return DashboardAction::ActivateInfo;
    case SDLK_TAB:return DashboardAction::ToggleLayout;
    case SDLK_d:return DashboardAction::ToggleTheme;
    case SDLK_u:return DashboardAction::ToggleUnits;
    default:return std::nullopt;
    }
}
} // namespace cluster::host
