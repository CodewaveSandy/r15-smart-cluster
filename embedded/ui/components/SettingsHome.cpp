#include "SettingsHome.h"
#include "../WidgetPrimitives.h"
#include "../Icons.h"
#include "embedded/presentation/BluetoothPresentation.h"
#include "embedded/application/WifiMenu.h"
namespace cluster::ui {
using namespace widget;
using namespace geometry;
SettingsHome::SettingsHome(lv_obj_t* parent):root_(box(parent,SettingsLayout::bounds)) {
    title_=label(root_.get(),SettingsLayout::title,"SETTINGS",fonts::heading);
    subtitle_=label(root_.get(),SettingsLayout::subtitle,"Appearance and connectivity");
    back_=label(root_.get(),SettingsLayout::back,"< Back",fonts::value); alignRight(back_);
    const char* names[]={"Appearance","Bluetooth","Wi-Fi","Navigation"};
    const char* details[]={"Theme · Design","Not configured","Not configured","Setup required"};
    const Icon icons[]={Icon::Appearance,Icon::Bluetooth,Icon::Wifi,Icon::Navigation};
    for(int i=0;i<4;++i) {
        tiles_[i]=box(root_.get(),SettingsLayout::tile(i));
        lv_obj_set_style_radius(tiles_[i],6,0);
        lv_obj_set_style_border_width(tiles_[i],2,0);
        icons_[i]=createIcon(tiles_[i],22,34,icons[i]);
        names_[i]=label(tiles_[i],78,23,265,names[i],fonts::heading);
        details_[i]=label(tiles_[i],78,58,265,details[i]);
    }
    show(root_.get(),false);
}
void SettingsHome::update(const ViewOptions& options,const BluetoothState& radio,const WifiState& wifi,const Palette& p,int progress) {
    show(root_.get(),progress>0&&!options.appearance&&!options.bluetooth&&!options.wifi);
    if(progress==0) return;
    const auto bluetooth=presentation::bluetoothPresentation(radio,false);
    text(details_[1],bluetooth.status);
    text(details_[2],wifi.available()?(wifi.connected?"Connected":"Disconnected"):"Status unavailable");
    const lv_opa_t alpha=progress*255/1000;
    for(auto* label:{title_,subtitle_,back_}) fade(label,alpha);
    ink(title_,p.ink); ink(subtitle_,p.muted); ink(back_,p.muted);
    for(unsigned i=0;i<4;++i) {
        const bool selected=i==options.settingsFocus;
        background(tiles_[i],p.panel);
        if(lv_obj_get_style_bg_opa(tiles_[i],0)!=alpha) lv_obj_set_style_bg_opa(tiles_[i],alpha,0);
        if(lv_obj_get_style_border_opa(tiles_[i],0)!=alpha) lv_obj_set_style_border_opa(tiles_[i],alpha,0);
        for(auto* label:{icons_[i],names_[i],details_[i]}) fade(label,alpha);
        const auto border=color(selected?p.accent:p.line);
        if(!lv_color_eq(lv_obj_get_style_border_color(tiles_[i],0),border)) lv_obj_set_style_border_color(tiles_[i],border,0);
        ink(icons_[i],selected?p.accent:p.muted);
        ink(names_[i],p.ink); ink(details_[i],i==1?(bluetooth.error?p.red:bluetooth.active?p.accent:p.muted):p.muted);
    }
}
}
