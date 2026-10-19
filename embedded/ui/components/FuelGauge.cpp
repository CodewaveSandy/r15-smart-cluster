#include "FuelGauge.h"
#include "../WidgetPrimitives.h"
#include "../Icons.h"
#include "embedded/presentation/DashboardPresentation.h"

namespace cluster::ui {
using namespace cluster::presentation;
using namespace widget;
using namespace geometry;
void FuelGauge::drawSegment(lv_event_t* event) {
    auto* obj=static_cast<lv_obj_t*>(lv_event_get_target(event));
    const auto& style=*static_cast<const SegmentStyle*>(lv_event_get_user_data(event));
    lv_area_t a; lv_obj_get_coords(obj,&a);
    const auto draw=[&](int fill,std::uint32_t colorValue) {
        if(fill<=0) return;
        // Both ends lean forward; fractional coverage moves within each piece.
        const int span=a.x2-a.x1-Metrics::fuelSkew;
        const int width=(span*fill+50)/100;
        lv_draw_triangle_dsc_t d; lv_draw_triangle_dsc_init(&d);
        d.bg_color=color(colorValue); d.bg_opa=255;
        d.p[0]={a.x1+Metrics::fuelSkew,a.y1};
        d.p[1]={a.x1+Metrics::fuelSkew+width,a.y1};
        d.p[2]={a.x1+width,a.y2};
        lv_draw_triangle(lv_event_get_layer(event),&d);
        d.p[1]=d.p[2]; d.p[2]={a.x1,a.y2};
        lv_draw_triangle(lv_event_get_layer(event),&d);
    };
    draw(100,style.empty);
    draw(style.fill,style.active);
}
FuelGauge::FuelGauge(lv_obj_t* parent) {
    fuelDivider_.reset(box(parent,FuelLayout::divider));
    fuelTitle_.reset(label(parent,FuelLayout::title,"FUEL"));
    fuelValue_.reset(label(parent,FuelLayout::value,"—",fonts::heading)); alignRight(fuelValue_.get());
    pump_.reset(createIcon(parent,FuelLayout::pump.x,FuelLayout::pump.y,Icon::Fuel));
    reserveMarker_.reset(box(parent,FuelLayout::reserveMarker));
    for(int i=0;i<Metrics::fuelSegmentCount;++i) {
        fuel_[i].reset(box(parent,FuelLayout::segment(i))); lv_obj_add_event_cb(fuel_[i].get(),drawSegment,LV_EVENT_DRAW_MAIN,&segmentStyles_[i]); }
}
void FuelGauge::update(const Signal<int>& fuelSignal,const Palette& p,std::uint64_t now) {
    const auto fuel=fuelPresentation(fuelSignal,now);
    const auto fuelColor=fuelSignal.available()&&fuelSignal.value<10?p.red:
        fuelSignal.available()&&fuelSignal.value<=20?p.amber:p.accent;
    const bool lit=fuel.lit;
    text(fuelTitle_.get(),fuel.reserve?"RESERVE":"FUEL");
    ink(fuelTitle_.get(),fuel.reserve?fuelColor:p.muted);
    paint(reserveMarker_.get(),p.amber);
    if(lastFuel_.update(fuelSignal)) fuelText_.update(fuelValue_.get(),fuelText(fuelSignal).value);
    ink(fuelValue_.get(),fuelColor);
    ink(pump_.get(),fuelColor); show(pump_.get(),lit);
    for(int i=0;i<Metrics::fuelSegmentCount;++i) {
        auto& style=segmentStyles_[i];
        const int fill=lit?fuel.segmentFill(i):0;
        if(style.fill!=fill || style.active!=fuelColor || style.empty!=p.panel) {
            style={fill,fuelColor,p.panel};
            lv_obj_invalidate(fuel_[i].get());
        }
    }
    paint(fuelDivider_.get(),p.line);
}
} // namespace cluster::ui
