#include "Dashboard.h"
#include "embedded/presentation/InstrumentPresentation.h"
#include "embedded/presentation/LightSignatureAnimation.h"
#include "WidgetPrimitives.h"
#include "embedded/application/SettingsNavigation.h"

namespace cluster::ui {
using namespace cluster::presentation;
using namespace widget;
Dashboard::Dashboard(lv_obj_t* parent)
    : root_(box(parent,0,0,Metrics::width,Metrics::height)),
      instruments_(box(root_.get(),0,0,Metrics::width,Metrics::height)),
      header_(root_.get()), tellTales_(root_.get()), tachometer_(instruments_.get()), settings_(root_.get()), appearance_(root_.get()), phone_(root_.get()), primary_(root_.get()),
      rideInfo_(instruments_.get()), fuel_(instruments_.get()), overlay_(root_.get()) {
    shiftGradient_.dir=LV_GRAD_DIR_VER; shiftGradient_.stops_count=3;
    const std::uint32_t shiftColors[]={0x492731,0xb34c48,0x64303b};
    for(int i=0;i<3;++i) { shiftGradient_.stops[i].color=color(shiftColors[i]); shiftGradient_.stops[i].opa=255; shiftGradient_.stops[i].frac=i*127; }
}
Dashboard::~Dashboard()=default;
void Dashboard::update(const ClusterState& s,const ViewOptions& v,const Lifecycle& life,std::uint64_t now) {
    const bool warning=urgent(s);
    const bool allowed=canOpenSettings(s,life);
    const int settingsProgress=settingsTransition_.update(v.settings&&allowed,now,!allowed);
    const bool shift=shiftLit(s.engineRpm,now);
    auto p=v.light?daylight:midnight;
    if(shift) { p.ink=midnight.ink; p.muted=0xf0d5d0; p.panel=0x352730; p.accent=midnight.accent;
        p.rpmBed=midnight.rpmBed; p.rpmGreen=midnight.rpmGreen; p.rpmAmber=midnight.rpmAmber; p.rpmRed=midnight.rpmRed; }
    if(!initialized_ || previousLight_!=v.light || previousShift_!=shift) {
        paint(root_.get(),shift?0x492731:p.background);
        lv_obj_set_style_bg_grad(root_.get(),shift?&shiftGradient_:nullptr,0);
        ink(root_.get(),p.ink);
    }
    const auto animation=animationFrame(life,warning,now);
    // The opaque lifecycle cover makes dashboard mutations invisible. Resume
    // directly from the latest snapshot on reveal; never replay a hidden backlog.
    if(animation.dashboardVisible) {
        header_.update(s,v,p,settingsProgress);
        // Hide the instrument group without a full-screen alpha layer. Only
        // the persistent speed label moves/scales between the two layouts.
        show(instruments_.get(),settingsProgress==0);
        settings_.update(v,s.phone,p,settingsProgress);
        appearance_.update(v,p,settingsProgress);
        phone_.update(s.phone,v,p,settingsProgress);
        tellTales_.update(s,p,now,v.iconTest);
        primary_.update(s,v,p,settingsProgress);
        if(settingsProgress==0) {
            tachometer_.update(s.engineRpm,p);
            rideInfo_.update(s,v,p);
            fuel_.update(s.fuelPercent,p,now);
        }
    }
    overlay_.update(animation);
    initialized_=true; previousLight_=v.light; previousShift_=shift;
}
} // namespace cluster::ui
