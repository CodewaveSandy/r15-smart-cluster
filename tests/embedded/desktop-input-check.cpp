#include "simulator/lvgl/DesktopInput.h"
#include <cstdio>
#include <utility>
int main() {
    using namespace cluster;
    const std::pair<SDL_Keycode,DashboardAction> mappings[]={
        {SDLK_RETURN,DashboardAction::NextTrip},{SDLK_i,DashboardAction::NextInfo},
        {SDLK_t,DashboardAction::ActivateInfo},{SDLK_TAB,DashboardAction::ToggleLayout},
        {SDLK_d,DashboardAction::ToggleTheme},{SDLK_u,DashboardAction::ToggleUnits}};
    for(const auto& mapping:mappings) {
        const auto action=host::dashboardAction(mapping.first);
        if(!action||*action!=mapping.second) { std::fprintf(stderr,"Desktop semantic mapping mismatch\n"); return 1; }
    }
    for(const auto key:{SDLK_UP,SDLK_RIGHT,SDLK_0,SDLK_o,SDLK_F2,SDLK_F3,SDLK_F4,SDLK_ESCAPE}) {
        if(host::dashboardAction(key)) { std::fprintf(stderr,"Simulator key consumed by navigation adapter\n"); return 1; }
    }
    if(host::settingsAction(SDLK_UP,false) || host::settingsAction(SDLK_ESCAPE,false)) return 1;
    for(const auto& mapping: {std::pair<SDL_Keycode,SettingsAction>{SDLK_s,SettingsAction::Open},
            {SDLK_RETURN,SettingsAction::Select},{SDLK_UP,SettingsAction::Up},{SDLK_DOWN,SettingsAction::Down},
            {SDLK_LEFT,SettingsAction::Left},{SDLK_RIGHT,SettingsAction::Right},
            {SDLK_BACKSPACE,SettingsAction::Back},{SDLK_ESCAPE,SettingsAction::Back}}) {
        const auto action=host::settingsAction(mapping.first,true);
        if(!action||*action!=mapping.second) return 1;
    }
    return 0;
}
