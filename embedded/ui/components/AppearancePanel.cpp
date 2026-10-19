#include "AppearancePanel.h"
#include "../WidgetPrimitives.h"
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
void thumbnail(lv_obj_t* parent,Rect bounds,bool light) {
    const auto p=light?daylight:midnight;
    auto* preview=box(parent,bounds); paint(preview,p.background);
    const int inner=bounds.width-16;
    for(int i=0;i<3;++i) paint(box(preview,8+i*inner/3,9,inner/3,6),i==0?0x4cdb88:i==1?0xf5d45b:0xf06464);
    auto* speed=label(preview,0,21,bounds.width,"60",fonts::preview);
    lv_obj_set_style_text_align(speed,LV_TEXT_ALIGN_CENTER,0); ink(speed,p.ink);
}
}
AppearancePanel::AppearancePanel(lv_obj_t* parent):root_(box(parent,AppearanceLayout::bounds)) {
    title_=label(root_.get(),AppearanceLayout::title,"APPEARANCE",fonts::heading);
    themeTitle_=label(root_.get(),AppearanceLayout::themeTitle,"Theme");
    back_=label(root_.get(),AppearanceLayout::back,"< Back",fonts::value); alignRight(back_);
    for(int i=0;i<2;++i) {
        themes_[i]=optionBox(root_.get(),AppearanceLayout::theme(i));
        thumbnail(themes_[i],{14,15,94,50},i==1);
        themeNames_[i]=label(themes_[i],126,16,220,i==0?"Midnight":"Daylight",fonts::heading);
        themeStatus_[i]=label(themes_[i],126,48,220,"Selected");
    }
    design_=optionBox(root_.get(),AppearanceLayout::design);
    thumbnail(design_,{14,12,130,66},false);
    designTitle_=label(design_,170,15,540,"Instrument",fonts::heading);
    designStatus_=label(design_,170,49,540,"Current design");
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
    text(status_,options.appearanceSaveFailed?"Could not save. Try Apply again.":
        options.light!=options.appearanceOriginalLight?"Preview - choose Apply to save":"Select a theme, then Apply");
    ink(status_,options.appearanceSaveFailed?p.red:p.muted);
}
}
