#include "FuelGauge.h"
#include "../WidgetPrimitives.h"
#include "../Icons.h"
#include "../InstrumentPresentation.h"
#include <cstdio>

namespace cluster::ui {
using namespace widget;
using namespace geometry;
namespace {
void drawFuelSegment(lv_event_t* event) {
    auto* obj=static_cast<lv_obj_t*>(lv_event_get_target(event));
    lv_area_t a; lv_obj_get_coords(obj,&a);
    lv_draw_triangle_dsc_t d; lv_draw_triangle_dsc_init(&d);
    d.bg_color=lv_obj_get_style_bg_color(obj,0); d.bg_opa=255;
    d.p[0]={a.x1+Metrics::fuelSkew,a.y1}; d.p[1]={a.x2,a.y1}; d.p[2]={a.x2-Metrics::fuelSkew,a.y2};
    lv_draw_triangle(lv_event_get_layer(event),&d);
    d.p[1]=d.p[2]; d.p[2]={a.x1,a.y2};
    lv_draw_triangle(lv_event_get_layer(event),&d);
}
}
FuelGauge::FuelGauge(lv_obj_t* parent) {
    fuelDivider_.reset(box(parent,FuelLayout::divider));
    fuelTitle_.reset(label(parent,FuelLayout::title,"F U E L"));
    fuelValue_.reset(label(parent,FuelLayout::value,"—",fonts::heading)); alignRight(fuelValue_.get());
    pump_.reset(createIcon(parent,FuelLayout::pump.x,FuelLayout::pump.y,Icon::Fuel));
    for(int i=0;i<Metrics::fuelSegmentCount;++i) {
        fuel_[i].reset(box(parent,FuelLayout::segment(i))); lv_obj_add_event_cb(fuel_[i].get(),drawFuelSegment,LV_EVENT_DRAW_MAIN,nullptr); }
}
void FuelGauge::update(const Signal<int>& fuelSignal,const Palette& p,std::uint64_t now) {
    ink(fuelTitle_.get(),p.muted);
    char b[16];
    const auto fuel=fuelPresentation(fuelSignal,now);
    const auto fuelColor=fuelSignal.available()&&fuelSignal.value<10?p.red:
        fuelSignal.available()&&fuelSignal.value<=20?p.amber:p.accent;
    const bool lit=fuel.lit;
    if(fuelSignal.available()) std::snprintf(b,sizeof b,"%d%%",fuelSignal.value); else std::snprintf(b,sizeof b,"—"); text(fuelValue_.get(),b); ink(fuelValue_.get(),fuelColor);
    ink(pump_.get(),fuelColor); show(pump_.get(),lit);
    const int segments=fuel.segments;
    for(int i=0;i<Metrics::fuelSegmentCount;++i) background(fuel_[i].get(),i<segments&&lit?fuelColor:p.panel);
    paint(fuelDivider_.get(),p.line);
}
} // namespace cluster::ui
