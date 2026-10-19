#pragma once
#include "embedded/application/DashboardInput.h"
#include <SDL_keycode.h>
#include <optional>
namespace cluster::host {
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
