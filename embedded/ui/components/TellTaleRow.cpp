#include "TellTaleRow.h"
#include "../WidgetPrimitives.h"
#include "../Icons.h"

namespace cluster::ui {
using namespace widget;
using namespace geometry;
TellTaleRow::TellTaleRow(lv_obj_t* parent) {
    lamps_[Left].reset(createIcon(parent,TellTaleLayout::x[Left],TellTaleLayout::y,Icon::Left));
    lamps_[Neutral].reset(label(parent,TellTaleLayout::neutral,"N",fonts::heading));
    lv_obj_set_style_text_align(lamps_[Neutral].get(),LV_TEXT_ALIGN_CENTER,0);
    lamps_[HighBeam].reset(createIcon(parent,TellTaleLayout::x[HighBeam],TellTaleLayout::y,Icon::HighBeam));
    lamps_[TractionControl].reset(createIcon(parent,TellTaleLayout::x[TractionControl],TellTaleLayout::y,Icon::TractionControl));
    lamps_[Abs].reset(createIcon(parent,TellTaleLayout::x[Abs],TellTaleLayout::y,Icon::Abs));
    lamps_[Engine].reset(createIcon(parent,TellTaleLayout::x[Engine],TellTaleLayout::y,Icon::Engine));
    lamps_[Coolant].reset(createIcon(parent,TellTaleLayout::x[Coolant],TellTaleLayout::y,Icon::Coolant));
    lamps_[Message].reset(createIcon(parent,TellTaleLayout::x[Message],TellTaleLayout::y,Icon::Message));
    lamps_[Call].reset(createIcon(parent,TellTaleLayout::x[Call],TellTaleLayout::y,Icon::Call));
    lamps_[Hazard].reset(createIcon(parent,TellTaleLayout::x[Hazard],TellTaleLayout::y,Icon::Hazard));
    lamps_[Right].reset(createIcon(parent,TellTaleLayout::x[Right],TellTaleLayout::y,Icon::Right));
}
void TellTaleRow::update(const ClusterState& s,const Palette& p,std::uint64_t now,bool iconTest) {
    const bool turn=(now/Metrics::turnPeriod)%2==0;
    statusLamp(lamps_[Left].get(),s.left,turn,p.accent,p,iconTest); statusLamp(lamps_[Right].get(),s.right,turn,p.accent,p,iconTest);
    statusLamp(lamps_[Neutral].get(),Signal<bool>{s.gear.value==0,s.gear.quality},true,p.accent,p,iconTest);
    statusLamp(lamps_[HighBeam].get(),s.highBeam,true,p.beam,p,iconTest); statusLamp(lamps_[Abs].get(),s.absWarning,true,p.amber,p,iconTest);
    statusLamp(lamps_[Engine].get(),s.engineWarning,true,p.amber,p,iconTest); statusLamp(lamps_[Coolant].get(),s.coolantWarning,true,p.red,p,iconTest);
    for(auto i:{TractionControl,Message,Call}) statusLamp(lamps_[i].get(),Signal<bool>{false,Quality::Unsupported},true,p.amber,p,iconTest);
    statusLamp(lamps_[Hazard].get(),Signal<bool>{s.left.value&&s.right.value,s.left.available()&&s.right.available()?Quality::Available:Quality::Unavailable},turn,p.red,p,iconTest);
}
} // namespace cluster::ui
