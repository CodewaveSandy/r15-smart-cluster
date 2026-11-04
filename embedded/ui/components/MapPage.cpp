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
    lv_label_set_long_mode(detail_,LV_LABEL_LONG_WRAP);lv_obj_set_height(detail_,38);
    coverage_=label(card_,14,94,216,"",fonts::caption);
    routeCard_=box(root_.get(),20,328,244,102);lv_obj_set_style_radius(routeCard_,10,0);
    guidance_=label(routeCard_,14,14,216,"",fonts::label);lv_obj_set_style_text_line_space(guidance_,8,0);show(routeCard_,false);
    lv_label_set_long_mode(guidance_,LV_LABEL_LONG_WRAP);lv_obj_set_height(guidance_,74);
    north_=label(root_.get(),758,152,22,"N",fonts::value);
    mode_=label(root_.get(),666,156,84,"",fonts::caption);
    scale_=label(root_.get(),680,414,100,"",fonts::caption);
    footer_=box(root_.get(),0,442,800,38);
    copyright_=label(footer_,12,11,18,"©",&inter_map_copyright_16);
    attribution_=label(footer_,32,12,278,"OpenStreetMap contributors",fonts::caption);
    const char* names[]={"Back","Follow","Area","-","+","Pan"};const int xs[]={314,388,474,550,612,674};const int widths[]={70,82,72,58,58,118};
    for(unsigned i=0;i<6;++i){controls_[i]=label(footer_,xs[i],10,widths[i],names[i],fonts::value);lv_obj_set_style_text_align(controls_[i],LV_TEXT_ALIGN_CENTER,0);}
    show(root_.get(),false);
}
void MapPage::update(const MapPageState& state,const ViewOptions& v,const Palette& p,bool visible){
    show(root_.get(),visible);if(!visible)return;
    panel_.bindScene(state.scene);panel_.update(state.pose,v.mapCamera.view(),v.light);
    const double ground=state.scene&&state.scene->mercator?std::cos(state.scene->originLatE7/1e7*maps::pi/180):1;
    char scale[24];std::snprintf(scale,sizeof scale,"%.0f m",96*ground/v.mapCamera.view().pixelsPerMetre);
    text(scale_,state.scene?scale:"");ink(scale_,p.muted);
    text(title_,state.transfer);text(detail_,state.location);
    text(coverage_,state.coverage);show(coverage_,state.navigation);lv_obj_set_height(card_,state.navigation?156:136);
    lv_obj_set_y(licence_,state.navigation?126:104);paint(routeCard_,p.panel);ink(guidance_,p.ink);text(guidance_,state.guidance);show(routeCard_,state.navigation&&state.guidance&&state.guidance[0]);
    text(attribution_,state.attribution);
    text(mode_,v.mapCamera.following()?"FOLLOW":"BROWSE");ink(mode_,p.muted);
    paint(card_,p.panel);paint(footer_,p.panel);ink(title_,p.ink);ink(detail_,p.muted);ink(north_,p.muted);ink(licence_,p.muted);ink(copyright_,p.ink);ink(attribution_,p.muted);
    show(copyright_,state.scene!=nullptr);show(attribution_,state.scene!=nullptr);
    const char* names[]={"Back","Follow","Area","-","+","Pan"};
    const char* panNames[]={"West","East","North","South","Done","Pan mode"};
    for(unsigned i=0;i<6;++i){
        text(controls_[i],v.mapPan?panNames[i]:names[i]);
        const bool focus=v.mapPan?i==v.mapPanFocus:i==v.mapFocus;
        paint(controls_[i],focus?p.accent:p.panel);ink(controls_[i],focus?p.background:p.ink);
    }
}
}
