#include "WifiPanel.h"
#include "../WidgetPrimitives.h"
#include "../Icons.h"
#include "embedded/application/WifiMenu.h"

namespace cluster::ui {
using namespace widget;
using namespace geometry;
namespace {
lv_obj_t* outlined(lv_obj_t* parent,Rect bounds) {
    auto* object=box(parent,bounds);
    lv_obj_set_style_radius(object,6,0); lv_obj_set_style_border_width(object,2,0);
    return object;
}
void outlineColor(lv_obj_t* object,std::uint32_t rgb) {
    const auto value=color(rgb);
    if(!lv_color_eq(lv_obj_get_style_border_color(object,0),value)) lv_obj_set_style_border_color(object,value,0);
}
}
WifiPanel::WifiPanel(lv_obj_t* parent):root_(box(parent,WifiLayout::bounds)) {
    title_=label(root_.get(),WifiLayout::title,"WI-FI",fonts::heading);
    subtitle_=label(root_.get(),WifiLayout::subtitle,"Network connection",fonts::caption);
    back_=label(root_.get(),WifiLayout::back,"< Back",fonts::value); alignRight(back_);
    card_=outlined(root_.get(),WifiLayout::device);
    icon_=createIcon(card_,20,24,Icon::Wifi);
    name_=label(card_,72,10,650,"Wifi unavailable",fonts::heading);
    status_=label(card_,72,37,650,"Status unavailable");
    for(unsigned i=0;i<5;++i) {
        rows_[i]=outlined(root_.get(),WifiLayout::row(i));
        labels_[i]=label(rows_[i],22,8,520,"Back",fonts::value);
        arrows_[i]=label(rows_[i],560,8,160,">",fonts::value); alignRight(arrows_[i]);
    }
    show(root_.get(),false);
}
void WifiPanel::update(const WifiState& state,const ViewOptions& view,const Palette& p,int progress) {
    show(root_.get(),view.wifi&&progress==1000);
    if(!view.wifi||progress!=1000) return;

    const auto menu=wifiMenu(state,view.wifiForgetConfirmation);
    text(subtitle_,"SIMULATED NETWORK SERVICE");
    ink(title_,p.ink); ink(subtitle_,p.muted); ink(back_,p.muted);
    paint(card_,p.panel); outlineColor(card_,p.line);
    text(name_,view.wifiCredentialNotice?"Protected network":view.wifiForgetConfirmation?"Forget connected network?":state.available()&&state.connected?wifiName(state.networkName):"Wi-Fi networks"); text(status_,view.wifiCredentialNotice?"Credentials pending - Enter simulates connection":wifiStatus(state));
    ink(name_,p.ink); ink(status_,(state.available()&&state.phase==WifiPhase::Failed)?p.red:(state.available()&&state.connected)?p.accent:p.muted);
    ink(icon_,(state.available()&&state.connected)?p.accent:p.muted);
    const auto focused=view.wifiCredentialNotice?0:view.wifiFocus%menu.count;
    for(unsigned i=0;i<5;++i) {
        show(rows_[i],i<(view.wifiCredentialNotice?1:menu.count));
        if(i>=(view.wifiCredentialNotice?1:menu.count)) continue;
        const auto item=menu.items[i];
        paint(rows_[i],p.panel); outlineColor(rows_[i],i==focused?p.accent:p.line);
        const char* title="Back";
        switch(item) {
        case WifiItem::Network:title=wifiName(state.networks[i].name);break;
        case WifiItem::Scan:title=state.count?"Search again":"Search networks";break;
        case WifiItem::Disconnect:title="Disconnect";break;
        case WifiItem::Forget:title="Forget network";break;
        case WifiItem::ConfirmForget:title="Forget network";break;
        case WifiItem::Cancel:title="Cancel";break;
        default:break;
        }
        text(labels_[i],view.wifiCredentialNotice?"Simulate connection":title);
        ink(labels_[i],item==WifiItem::ConfirmForget?p.red:p.ink);
        text(arrows_[i],!view.wifiCredentialNotice&&item==WifiItem::Network?(state.networks[i].saved?"Saved":state.networks[i].secured?"Protected":"Open"):">");
        ink(arrows_[i],i==focused?p.accent:p.muted);
    }
}
}
