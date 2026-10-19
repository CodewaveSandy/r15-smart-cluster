#include "LifecycleOverlay.h"
#include "../WidgetPrimitives.h"
#include <algorithm>

namespace cluster::ui {
using namespace widget;
using namespace geometry;
LifecycleOverlay::LifecycleOverlay(lv_obj_t* parent) {
    overlay_.reset(box(parent,LifecycleLayout::bounds));
    animationTitle_=label(overlay_.get(),LifecycleLayout::title,"R15",fonts::secondary);
    lv_obj_set_style_text_align(animationTitle_,LV_TEXT_ALIGN_CENTER,0);
    animationSubtitle_=label(overlay_.get(),LifecycleLayout::subtitle,"SMART CLUSTER");
    lv_obj_set_style_text_align(animationSubtitle_,LV_TEXT_ALIGN_CENTER,0);
    animationBed_=box(overlay_.get(),LifecycleLayout::progress);
    animationFill_=box(animationBed_,LifecycleLayout::fill);
}
void LifecycleOverlay::update(const Lifecycle& life,bool urgentWarning,const Palette& p,std::uint64_t now) {
    const auto phase=life.phase();
    const bool animation=phase==Phase::Boot||phase==Phase::Shutdown;
    const bool cover=phase!=Phase::Running&&!(animation&&urgentWarning);
    show(overlay_.get(),cover); paint(overlay_.get(),p.background);
    text(animationTitle_,phase==Phase::Boot?"R15":phase==Phase::Shutdown?"R15":"DISPLAY OFF");
    text(animationSubtitle_,phase==Phase::Boot?"SMART CLUSTER  /  STARTING":phase==Phase::Shutdown?"ENDING THE RIDE":"PRESS O TO TURN THE KEY ON");
    ink(animationSubtitle_,p.muted); paint(animationBed_,p.panel); paint(animationFill_,p.accent);
    show(animationBed_,animation);
    if(cover && animation) lv_obj_set_width(animationFill_,std::max(1,int(std::min(life.elapsed(now),Lifecycle::animationMs)*LifecycleLayout::progress.width/Lifecycle::animationMs)));
}
} // namespace cluster::ui
