#include "SettingsHome.h"
#include "../WidgetPrimitives.h"
#include "../Icons.h"
namespace cluster::ui {
using namespace widget;
using namespace geometry;
SettingsHome::SettingsHome(lv_obj_t* parent):root_(box(parent,SettingsLayout::bounds)) {
    title_=label(root_.get(),SettingsLayout::title,"SETTINGS",fonts::heading);
    subtitle_=label(root_.get(),SettingsLayout::subtitle,"Appearance and connectivity");
    back_=label(root_.get(),SettingsLayout::back,"< Back",fonts::value); alignRight(back_);
    const char* names[]={"Appearance","Phone","Navigation"};
    const char* details[]={"Theme · Design","Not connected","Setup required"};
    const Icon icons[]={Icon::Appearance,Icon::Phone,Icon::Navigation};
    for(int i=0;i<3;++i) {
        tiles_[i]=box(root_.get(),SettingsLayout::tile(i));
        lv_obj_set_style_radius(tiles_[i],6,0);
        lv_obj_set_style_border_width(tiles_[i],2,0);
        icons_[i]=createIcon(tiles_[i],22,34,icons[i]);
        names_[i]=label(tiles_[i],78,23,265,names[i],fonts::heading);
        details_[i]=label(tiles_[i],78,58,265,details[i]);
    }
    show(root_.get(),false);
}
void SettingsHome::update(const ViewOptions& options,const PhoneState& phone,const Palette& p,int progress) {
    show(root_.get(),progress==1000&&!options.appearance&&!options.bluetooth&&!options.wifi&&!options.phone);
    if(progress!=1000) return;
    text(details_[1],!phone.available()?"Status unavailable":phone.connected?"Connected":phone.paired?"Waiting for saved phone":"Pair your phone");
    const lv_opa_t alpha=progress*255/1000;
    for(auto* label:{title_,subtitle_,back_}) fade(label,alpha);
    ink(title_,p.ink); ink(subtitle_,p.muted); ink(back_,p.muted);
    for(unsigned i=0;i<3;++i) {
        const bool selected=i==options.settingsFocus;
        background(tiles_[i],p.panel);
        if(lv_obj_get_style_bg_opa(tiles_[i],0)!=alpha) lv_obj_set_style_bg_opa(tiles_[i],alpha,0);
        if(lv_obj_get_style_border_opa(tiles_[i],0)!=alpha) lv_obj_set_style_border_opa(tiles_[i],alpha,0);
        for(auto* label:{icons_[i],names_[i],details_[i]}) fade(label,alpha);
        const auto border=color(selected?p.accent:p.line);
        if(!lv_color_eq(lv_obj_get_style_border_color(tiles_[i],0),border)) lv_obj_set_style_border_color(tiles_[i],border,0);
        ink(icons_[i],selected?p.accent:p.muted);
        ink(names_[i],p.ink); ink(details_[i],i==1&&phone.available()&&phone.connected?p.accent:p.muted);
    }
}
}
