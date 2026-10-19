#include "BluetoothPanel.h"
#include "../WidgetPrimitives.h"
#include "../Icons.h"
#include "embedded/application/BluetoothMenu.h"
#include "embedded/presentation/BluetoothPresentation.h"
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
BluetoothPanel::BluetoothPanel(lv_obj_t* parent):root_(box(parent,BluetoothLayout::bounds)) {
    title_=label(root_.get(),BluetoothLayout::title,"BLUETOOTH",fonts::heading);
    subtitle_=label(root_.get(),BluetoothLayout::subtitle,"Phone connection",fonts::caption);
    back_=label(root_.get(),BluetoothLayout::back,"< Back",fonts::value); alignRight(back_);
    card_=outlined(root_.get(),BluetoothLayout::device);
    phone_=createIcon(card_,20,24,Icon::Phone);
    name_=label(card_,72,10,650,"Bluetooth unavailable",fonts::heading);
    status_=label(card_,72,46,650,"Status unavailable");
    for(unsigned i=0;i<3;++i) {
        rows_[i]=outlined(root_.get(),BluetoothLayout::row(i));
        labels_[i]=label(rows_[i],22,15,650,"Back",fonts::value);
        arrows_[i]=label(rows_[i],688,15,32,">",fonts::value); alignRight(arrows_[i]);
    }
    show(root_.get(),false);
}
void BluetoothPanel::update(const BluetoothState& state,const ViewOptions& view,const Palette& p,int progress) {
    show(root_.get(),view.bluetooth&&progress==1000);
    if(!view.bluetooth||progress!=1000) return;
    const auto presentation=presentation::bluetoothPresentation(state,view.bluetoothForgetConfirmation);
    const auto menu=bluetoothMenu(state,view.bluetoothForgetConfirmation);
    text(subtitle_,view.bluetoothDemo?"SIMULATED PHONE SERVICE":"Phone connection");
    ink(title_,p.ink); ink(subtitle_,p.muted); ink(back_,p.muted);
    paint(card_,p.panel); outlineColor(card_,p.line);
    text(name_,presentation.name.value); text(status_,presentation.status);
    ink(name_,p.ink); ink(status_,presentation.error?p.red:presentation.active?p.accent:p.muted);
    ink(phone_,presentation.active?p.accent:p.muted);
    const auto focused=view.bluetoothFocus%menu.count;
    for(unsigned i=0;i<3;++i) {
        show(rows_[i],i<menu.count);
        if(i>=menu.count) continue;
        const auto item=menu.items[i];
        paint(rows_[i],p.panel); outlineColor(rows_[i],i==focused?p.accent:p.line);
        text(labels_[i],bluetoothItemTitle(item,state.phase==BluetoothPhase::Failed&&!state.connected));
        ink(labels_[i],item==BluetoothItem::ConfirmForget?p.red:p.ink);
        ink(arrows_[i],i==focused?p.accent:p.muted);
    }
}
}
