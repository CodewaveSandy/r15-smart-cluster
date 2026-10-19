#include "Tachometer.h"
#include "../WidgetPrimitives.h"
#include <algorithm>
#include <cstdio>
#include "embedded/presentation/DashboardPresentation.h"

namespace cluster::ui {
using namespace widget;
using namespace geometry;
Tachometer::Tachometer(lv_obj_t* parent) {
    for(int i=0;i<TachometerLayout::tickCount;++i) { char b[8]; std::snprintf(b,sizeof b,"%d",i); ticks_[i].reset(label(parent,TachometerLayout::tick(i),b,fonts::caption)); }
    rpmBed_.reset(box(parent,TachometerLayout::bed));
    rpmClip_=box(rpmBed_.get(),TachometerLayout::clip);
    rpmRamp_=box(rpmClip_,TachometerLayout::ramp);
    rampGradient_.dir=LV_GRAD_DIR_HOR; rampGradient_.stops_count=6;
    const std::uint32_t colors[]={0x4cdb88,0x4cdb88,0xf5d45b,0xf5d45b,0xf06464,0xf06464};
    const int stops[]={0,3500,4500,7500,8500,13000};
    for(int i=0;i<6;++i) { rampGradient_.stops[i].color=color(colors[i]); rampGradient_.stops[i].opa=255; rampGradient_.stops[i].frac=stops[i]*255/TachometerLayout::scaleMaxRpm; }
    paint(rpmRamp_,colors[0]); lv_obj_set_style_bg_grad(rpmRamp_,&rampGradient_,0);
    caption_.reset(label(parent,TachometerLayout::caption,"RPM × 1,000",fonts::value));
    rpmValue_.reset(label(parent,TachometerLayout::value,"— rpm",fonts::value)); alignRight(rpmValue_.get());
}
void Tachometer::update(const Signal<int>& rpm,const Palette& p) {
    for(const auto& t:ticks_) ink(t.get(),p.muted);
    paint(rpmBed_.get(),p.panel);
    const int fill=rpm.available()?std::clamp(rpm.value,0,TachometerLayout::scaleMaxRpm)*Metrics::content/TachometerLayout::scaleMaxRpm:0;
    show(rpmClip_,fill>0); lv_obj_set_width(rpmClip_,std::max(1,fill));
    if(lastRpm_.update(rpm)) text(rpmValue_.get(),presentation::rpmText(rpm).value);
}
} // namespace cluster::ui
