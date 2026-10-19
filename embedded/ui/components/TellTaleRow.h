#pragma once
#include "lvgl.h"
#include "../ObjectOwner.h"
#include "../Tokens.h"
#include "shared/model/ClusterState.h"
#include <array>

namespace cluster::ui {
// Owns its top-level LVGL objects and their descendants. The supplied parent
// must outlive this component. All operations run on the LVGL owner thread.
class TellTaleRow {
public:
    explicit TellTaleRow(lv_obj_t* parent);
    TellTaleRow(const TellTaleRow&)=delete;
    TellTaleRow& operator=(const TellTaleRow&)=delete;
    void update(const ClusterState&,const Palette&,std::uint64_t now,bool iconTest);
private:
    enum Lamp { Left, Neutral, HighBeam, TractionControl, Abs, Engine, Coolant, Message, Call, Hazard, Right, Count };
    std::array<ObjectOwner,Count> lamps_{};
};
} // namespace cluster::ui
