#pragma once
#include "Geometry.h"
#include "embedded/application/SettingsNavigation.h"
#include <optional>

namespace cluster::ui {
// Coordinate adapter only: semantic reducers still enforce all access gates.
inline std::optional<SettingsEffect> settingsTouch(DisplayOptions& view,const ClusterState& state,
        const Lifecycle& life,int x,int y,bool menuReady) {
    using namespace geometry;
    const auto inside=[&](Rect r){return x>=r.x&&x<r.x+r.width&&y>=r.y&&y<r.y+r.height;};
    if(x<0||x>=800||y<0||y>=480) return std::nullopt;
    if(inside(HeaderLayout::settings))
        return settingsAction(view,view.settings?SettingsAction::Back:SettingsAction::Open,state,life);
    if(!view.settings) return std::nullopt;
    if(!canOpenSettings(state,life)) return reconcileSettings(view,state,life);
    if(!menuReady) return SettingsEffect::None;
    if(view.map){
        const int xs[]={340,430,530,620,690},widths[]={80,90,80,60,60};
        if(y>=442&&y<480)for(unsigned i=0;i<5;++i)if(x>=xs[i]&&x<xs[i]+widths[i]){view.mapFocus=i;return settingsAction(view,SettingsAction::Select,state,life);}
        return SettingsEffect::None;
    }
    y-=SettingsLayout::bounds.y;
    if(view.phone&&state.phone.available()&&valid(state.phone)&&state.phone.pairingRequested) {
        // Modal consumes backdrop taps; only explicit buttons can approve/reject.
        if(inside(PairingLayout::reject))view.phoneFocus=0;
        else if(!state.phone.pairingApproved&&inside(PairingLayout::allow))view.phoneFocus=1;
        else return SettingsEffect::None;
        return settingsAction(view,SettingsAction::Select,state,life);
    }
    if(inside({680,0,100,44})) return settingsAction(view,SettingsAction::Back,state,life);
    int focus=-1;
    if(view.clock) {
        for(unsigned i=0;i<5;++i)if(inside(ClockLayout::row(i)))focus=int(i);
        if(inside(AppearanceLayout::cancel))focus=5;
        if(inside(AppearanceLayout::apply))focus=6;
        if(focus>=0)view.clockFocus=unsigned(focus);
    } else if(view.appearance) {
        for(int i=0;i<2;++i) if(inside(AppearanceLayout::theme(i))) focus=i;
        if(inside(AppearanceLayout::design)) focus=2;
        if(inside(AppearanceLayout::cancel)) focus=3;
        if(inside(AppearanceLayout::apply)) focus=4;
        if(focus>=0) view.appearanceFocus=unsigned(focus);
    } else if(view.phone) {
        const auto menu=phoneMenu(state.phone,view.phoneForgetConfirmation);
        for(unsigned i=0;i<menu.count;++i) if(inside(PhoneLayout::row(i))) focus=int(i);
        if(focus>=0) view.phoneFocus=unsigned(focus);
    } else {
        for(int i=0;i<4;++i) if(inside(SettingsLayout::tile(i))) focus=i;
        if(focus>=0) view.settingsFocus=unsigned(focus);
    }
    return focus>=0?settingsAction(view,SettingsAction::Select,state,life):SettingsEffect::None;
}
}
