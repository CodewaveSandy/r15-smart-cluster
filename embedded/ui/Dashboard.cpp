#include "Dashboard.h"
#include "InstrumentPresentation.h"
#include "WidgetPrimitives.h"

namespace cluster::ui {
using namespace widget;
Dashboard::Dashboard(lv_obj_t* parent)
    : root_(box(parent,0,0,Metrics::width,Metrics::height)),
      header_(root_.get()), tellTales_(root_.get()), tachometer_(root_.get()), primary_(root_.get()),
      rideInfo_(root_.get()), fuel_(root_.get()), overlay_(root_.get()) {
    shiftGradient_.dir=LV_GRAD_DIR_VER; shiftGradient_.stops_count=3;
    const std::uint32_t shiftColors[]={0x492731,0xb34c48,0x64303b};
    for(int i=0;i<3;++i) { shiftGradient_.stops[i].color=color(shiftColors[i]); shiftGradient_.stops[i].opa=255; shiftGradient_.stops[i].frac=i*127; }}
Dashboard::~Dashboard()=default;
void Dashboard::update(const ClusterState& s,const ViewOptions& v,const Lifecycle& life,std::uint64_t now) {
    const bool shift=shiftLit(s.engineRpm,now);
    auto p=v.light?daylight:midnight;
    if(shift) { p.ink=midnight.ink; p.muted=0xf0d5d0; p.panel=0x352730; p.accent=midnight.accent; }
    if(!initialized_ || previousLight_!=v.light || previousShift_!=shift) {
        paint(root_.get(),shift?0x492731:p.background);
        lv_obj_set_style_bg_grad(root_.get(),shift?&shiftGradient_:nullptr,0);
        ink(root_.get(),p.ink);
    }
    header_.update(s,v,p);
    tellTales_.update(s,p,now,v.iconTest);
    tachometer_.update(s.engineRpm,p);
    primary_.update(s,v,p);
    rideInfo_.update(s,v,p);
    fuel_.update(s.fuelPercent,p,now);
    overlay_.update(life,urgent(s),p,now);
    initialized_=true; previousLight_=v.light; previousShift_=shift;
}
} // namespace cluster::ui
