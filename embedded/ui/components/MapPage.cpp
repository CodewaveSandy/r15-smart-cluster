#include "MapPage.h"
#include "../WidgetPrimitives.h"
extern "C" {LV_FONT_DECLARE(inter_map_copyright_16);}
namespace cluster::ui {
using namespace widget;
MapPage::MapPage(lv_obj_t* parent):root_(box(parent,0,0,800,480)),panel_(root_.get()){
    lv_obj_set_style_bg_opa(root_.get(),LV_OPA_TRANSP,0);
    card_=box(root_.get(),20,160,244,136);lv_obj_set_style_radius(card_,10,0);
    licence_=label(card_,14,104,216,"ODbL · openstreetmap.org/copyright",fonts::caption);
    title_=label(card_,14,16,216,"",fonts::value);detail_=label(card_,14,56,216,"",fonts::label);lv_obj_set_style_text_line_space(detail_,6,0);
    north_=label(root_.get(),758,152,22,"N",fonts::value);
    scale_=label(root_.get(),680,414,100,"",fonts::caption);
    footer_=box(root_.get(),0,442,800,38);
    copyright_=label(footer_,12,11,18,"©",&inter_map_copyright_16);
    attribution_=label(footer_,32,12,290,"OpenStreetMap contributors",fonts::caption);
    const char* names[]={"Back","Follow","Area","-","+"};const int xs[]={340,430,530,620,690};const int widths[]={80,90,80,60,60};
    for(unsigned i=0;i<5;++i){controls_[i]=label(footer_,xs[i],10,widths[i],names[i],fonts::value);lv_obj_set_style_text_align(controls_[i],LV_TEXT_ALIGN_CENTER,0);}
    show(root_.get(),false);
}
void MapPage::update(const MapPageState& state,const ViewOptions& v,const Palette& p,bool visible){
    show(root_.get(),visible);if(!visible)return;
    if(bound_!=state.scene){center_={};bound_=state.scene;}
    panel_.bindScene(state.scene);auto pose=state.pose;
    if(pose.quality==Quality::Available)center_=pose.point;else pose.point=center_;
    panel_.update(pose,v.mapOverview,v.light,v.mapZoom,pose.quality!=Quality::Available);
    text(scale_,!state.scene||v.mapOverview?"":v.mapZoom==1?"200 m":v.mapZoom==2?"100 m":"~67 m");ink(scale_,p.muted);
    text(title_,state.transfer);text(detail_,state.location);
    paint(card_,p.panel);paint(footer_,p.panel);ink(title_,p.ink);ink(detail_,p.muted);ink(north_,p.muted);ink(licence_,p.muted);ink(copyright_,p.ink);ink(attribution_,p.muted);
    show(copyright_,state.scene!=nullptr);show(attribution_,state.scene!=nullptr);
    for(unsigned i=0;i<5;++i){paint(controls_[i],i==v.mapFocus?p.accent:p.panel);ink(controls_[i],i==v.mapFocus?p.background:p.ink);}
}
}
