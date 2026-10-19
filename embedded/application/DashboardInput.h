#pragma once
#include "DisplayOptions.h"
namespace cluster {
enum class DashboardAction { NextTrip, NextInfo, ActivateInfo, ToggleLayout, ToggleTheme, ToggleUnits };
enum class InputEffect { None, RequestTcsToggle };
// Semantic actions modify display state only. Vehicle requests leave through a
// typed effect; the host may simulate them, a future transport must validate them.
inline InputEffect applyAction(DisplayOptions& options,DashboardAction action) {
    switch(action) {
    case DashboardAction::NextTrip:
        if(!options.track) options.tripPage=next(options.tripPage);
        break;
    case DashboardAction::NextInfo: options.infoPage=next(options.infoPage); break;
    case DashboardAction::ActivateInfo:
        if(options.infoPage==InfoPage::Track) options.track=!options.track;
        else if(options.infoPage==InfoPage::Tcs) return InputEffect::RequestTcsToggle;
        break;
    case DashboardAction::ToggleLayout: options.track=!options.track; break;
    case DashboardAction::ToggleTheme: options.light=!options.light; break;
    case DashboardAction::ToggleUnits: options.imperial=!options.imperial; break;
    }
    return InputEffect::None;
}
} // namespace cluster
