#pragma once
#include "../ObjectOwner.h"
#include "../Tokens.h"
#include "embedded/presentation/TurnGuidancePresentation.h"
namespace cluster::ui {
class TurnGuidanceCard {
public:
    explicit TurnGuidanceCard(lv_obj_t* parent);
    void update(const presentation::TurnCard&,const Palette&,bool visible,bool simulated);
private:
    guidance::Maneuver previous_=guidance::Maneuver::Unknown;std::uint32_t lineColor_=0,borderColor_=0;bool glyphInitialized_=false;
    ObjectOwner root_;lv_obj_t *source_,*instruction_,*road_,*summary_,*distance_,*neutral_,*path_,*head_,*exit_;
};
}
