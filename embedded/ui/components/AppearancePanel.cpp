#include "AppearancePanel.h"
#include "../WidgetPrimitives.h"
#include "Header.h"
#include "TellTaleRow.h"
#include "Tachometer.h"
#include "PrimaryReadout.h"
#include "RideInfoPanel.h"
#include "FuelGauge.h"
namespace cluster::ui {
using namespace widget;
using namespace geometry;
namespace {
void focusBox(lv_obj_t* object,const Palette& p,bool focused) {
    paint(object,p.panel);
    const auto border=color(focused?p.accent:p.line);
    if(!lv_color_eq(lv_obj_get_style_border_color(object,0),border)) lv_obj_set_style_border_color(object,border,0);
}
lv_obj_t* optionBox(lv_obj_t* parent,Rect bounds) {
    auto* object=box(parent,bounds);
    lv_obj_set_style_radius(object,6,0); lv_obj_set_style_border_width(object,2,0);
    return object;
}
}
// The preview composes the actual instruments, using fixed, clearly illustrative
// observations. No nested dashboard/menus, timers, transport or persistence.
struct AppearancePanel::Preview {
    ObjectOwner root;
    Header header; TellTaleRow lamps; Tachometer rpm; PrimaryReadout primary;
    RideInfoPanel ride; FuelGauge fuel;
    bool initialized=false,light=false,track=false,imperial=false;
    explicit Preview(lv_obj_t* parent):root(box(parent,79,5,800,480)),
        header(root.get()),lamps(root.get()),rpm(root.get()),primary(root.get()),ride(root.get()),fuel(root.get()) {
        lv_obj_set_style_transform_pivot_x(root.get(),0,0);
        lv_obj_set_style_transform_pivot_y(root.get(),0,0);
        lv_obj_set_style_transform_scale_x(root.get(),112,0);
        lv_obj_set_style_transform_scale_y(root.get(),112,0);
    }
    void update(const ViewOptions& options) {
        if(initialized&&light==options.light&&track==options.track&&imperial==options.imperial) return;
        initialized=true;light=options.light;track=options.track;imperial=options.imperial;
        const auto p=options.light?daylight:midnight;
        paint(root.get(),p.background);
        ClusterState s; s.speedKph={60,Quality::Available};s.engineRpm={8500,Quality::Available};
        s.gear={4,Quality::Available};s.fuelPercent={72,Quality::Available};
        s.currentLapMs={84380,Quality::Available};s.bestLapMs={78420,Quality::Available};
        s.odometerKm={12480,Quality::Available};s.coolantTemperatureC={84,Quality::Available};
        s.left=s.right=s.highBeam=s.absWarning=s.engineWarning=s.coolantWarning={false,Quality::Available};
        ViewOptions v;v.light=options.light;v.track=options.track;v.imperial=options.imperial;
        v.time="20:55";v.date="PREVIEW";
        header.update(s,v,p);lamps.update(s,p,0,false);rpm.update(s.engineRpm,p);
        primary.update(s,v,p,0);ride.update(s,v,p);fuel.update(s.fuelPercent,p,0);
    }
};
AppearancePanel::~AppearancePanel()=default;
AppearancePanel::AppearancePanel(lv_obj_t* parent):root_(box(parent,AppearanceLayout::bounds)) {
    title_=label(root_.get(),AppearanceLayout::title,"APPEARANCE",fonts::heading);
    themeTitle_=label(root_.get(),AppearanceLayout::themeTitle,"Theme");
    back_=label(root_.get(),AppearanceLayout::back,"< Back",fonts::value); alignRight(back_);
    for(int i=0;i<2;++i) {
        themes_[i]=optionBox(root_.get(),AppearanceLayout::theme(i));
        themeNames_[i]=label(themes_[i],16,12,188,i==0?"Midnight":"Daylight",fonts::heading);
        themeStatus_[i]=label(themes_[i],16,44,188,"Selected");
    }
    design_=optionBox(root_.get(),AppearanceLayout::design);
    preview_=std::make_unique<Preview>(design_);
    designTitle_=label(root_.get(),28,244,220,"Instrument",fonts::value);
    designStatus_=label(root_.get(),264,42,508,"LIVE PREVIEW",fonts::caption);
    buttons_[0]=optionBox(root_.get(),AppearanceLayout::cancel);
    buttons_[1]=optionBox(root_.get(),AppearanceLayout::apply);
    for(int i=0;i<2;++i) {
        buttonText_[i]=label(buttons_[i],0,11,136,i==0?"Cancel":"Apply",fonts::value);
        lv_obj_set_style_text_align(buttonText_[i],LV_TEXT_ALIGN_CENTER,0);
    }
    status_=label(root_.get(),AppearanceLayout::status,"Select a theme, then Apply");
    show(root_.get(),false);
}
void AppearancePanel::update(const ViewOptions& options,const Palette& p,int progress) {
    show(root_.get(),options.appearance&&progress==1000);
    if(!options.appearance||progress!=1000) return;
    preview_->update(options);
    ink(title_,p.ink); ink(themeTitle_,p.muted); ink(back_,p.muted);
    for(unsigned i=0;i<2;++i) {
        focusBox(themes_[i],p,options.appearanceFocus==i);
        ink(themeNames_[i],p.ink); ink(themeStatus_[i],p.accent);
        show(themeStatus_[i],options.light==(i==1));
    }
    focusBox(design_,p,options.appearanceFocus==2);
    ink(designTitle_,p.ink); ink(designStatus_,p.muted);
    for(unsigned i=0;i<2;++i) {
        focusBox(buttons_[i],p,options.appearanceFocus==i+3);
        ink(buttonText_[i],options.appearanceFocus==i+3?p.accent:p.ink);
    }
    text(status_,options.appearanceSaving?"Saving...":options.appearanceSaveFailed?"Could not save. Try Apply again.":
        options.light!=options.appearanceOriginalLight?"Preview - choose Apply to save":"Select a theme, then Apply");
    ink(status_,options.appearanceSaveFailed?p.red:p.muted);
}
}
