#include "PhonePanel.h"
#include "../WidgetPrimitives.h"
#include "../Icons.h"
#include "embedded/application/PhoneMenu.h"

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
PhonePanel::PhonePanel(lv_obj_t* parent):root_(box(parent,PhoneLayout::bounds)) {
    title_=label(root_.get(),PhoneLayout::title,"PHONE",fonts::heading);
    subtitle_=label(root_.get(),PhoneLayout::subtitle,"Phone connection",fonts::caption);
    back_=label(root_.get(),PhoneLayout::back,"< Back",fonts::value); alignRight(back_);
    card_=outlined(root_.get(),PhoneLayout::device);
    phone_=createIcon(card_,20,24,Icon::Phone);
    name_=label(card_,72,10,650,"Phone unavailable",fonts::heading);
    status_=label(card_,72,46,650,"Status unavailable",fonts::value);
    for(unsigned i=0;i<3;++i) {
        rows_[i]=outlined(root_.get(),PhoneLayout::row(i));
        labels_[i]=label(rows_[i],22,15,650,"Back",fonts::value);
        arrows_[i]=label(rows_[i],688,15,32,">",fonts::value); alignRight(arrows_[i]);
    }
    show(root_.get(),false);
}
void PhonePanel::update(const PhoneState& state,const ViewOptions& view,const Palette& p,int progress) {
    show(root_.get(),view.phone&&progress==1000);
    if(!view.phone||progress!=1000) return;
    const auto menu=phoneMenu(state,view.phoneForgetConfirmation);
    text(subtitle_,state.bleBench?"BLE bench pairing - phone services pending":"One phone connection for navigation and media");
    ink(title_,p.ink);ink(subtitle_,p.muted);ink(back_,p.muted);
    paint(card_,p.panel);outlineColor(card_,p.line);
    const bool ready=state.available()&&valid(state);
    text(name_,!ready?"Phone service unavailable":state.pairingRequested?"Pair this phone?":state.paired?"Rider phone":"Connect your phone");
    text(status_,!ready?"Waiting for phone service":state.pairingRequested?(state.pairingApproved?"Waiting for phone to finish pairing":state.pairingCode.data()):state.connected?"Connected - automatic reconnect enabled":state.paired?"Saved phone - reconnects when available":state.pairingCode[0]?state.pairingCode.data():"Open R15 Companion and select Connect cluster");
    if(state.bleBench) {
        const char* unavailable=state.unavailable==PhoneUnavailable::ReturnChannel?"Return UART not confirmed - check TX43 to GPIO18":
            state.unavailable==PhoneUnavailable::Status?"Waiting for vehicle phone-status packets":
            state.unavailable==PhoneUnavailable::VehicleSession?"Waiting for matching vehicle session":"Vehicle BLE service unavailable";
        text(status_,!ready?unavailable:state.forgetting?"Disconnecting and forgetting phone...":state.commandFailed?"Request failed. Try again.":
            state.pairingRequested?(state.pairingApproved?"Waiting for phone confirmation":state.pairingCode.data()):
            state.connected?"Authenticated BLE bond connected":state.paired?"Saved BLE bond - reconnect from phone":
            state.pairWindowOpen?"Pairing enabled for 60 seconds - connect from BLE app":"Select Pair phone, then use a BLE test app");
    }
    ink(name_,p.ink);ink(status_,ready&&state.connected?p.accent:p.muted);ink(phone_,ready&&state.connected?p.accent:p.muted);
    const auto focused=view.phoneFocus%menu.count;
    for(unsigned i=0;i<3;++i) {
        show(rows_[i],i<menu.count);
        if(i>=menu.count) continue;
        const auto item=menu.items[i];
        paint(rows_[i],p.panel); outlineColor(rows_[i],i==focused?p.accent:p.line);
        text(labels_[i],phoneItemTitle(item));
        ink(labels_[i],item==PhoneItem::ConfirmForget?p.red:p.ink);
        ink(arrows_[i],i==focused?p.accent:p.muted);
    }
}
}
