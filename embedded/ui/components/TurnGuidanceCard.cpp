#include "TurnGuidanceCard.h"
#include "../WidgetPrimitives.h"
namespace cluster::ui {
using namespace widget;
namespace {
// Local geometry, independent of notification icons/resource identifiers.
const lv_point_precise_t straight[]={{29,49},{29,7}},straightHead[]={{17,19},{29,7},{41,19}};
const lv_point_precise_t left[]={{43,49},{43,22},{12,22}},leftHead[]={{24,10},{12,22},{24,34}};
const lv_point_precise_t right[]={{15,49},{15,22},{46,22}},rightHead[]={{34,10},{46,22},{34,34}};
const lv_point_precise_t keepLeft[]={{43,49},{43,35},{17,9}},keepLeftHead[]={{17,27},{17,9},{35,9}};
const lv_point_precise_t keepRight[]={{15,49},{15,35},{41,9}},keepRightHead[]={{23,9},{41,9},{41,27}};
const lv_point_precise_t uturn[]={{43,49},{43,17},{37,8},{22,8},{15,17},{15,39}},uturnHead[]={{4,27},{15,39},{26,27}};
const lv_point_precise_t roundabout[]={{29,49},{29,41},{14,35},{9,23},{16,10},{29,6},{42,12},{47,25},{41,37},{29,41}},roundHead[]={{29,6},{29,0},{39,0}};
const lv_point_precise_t arrive[]={{16,48},{16,6},{43,6},{43,25},{16,25}},arriveHead[]={{18,8},{40,23}};
}
TurnGuidanceCard::TurnGuidanceCard(lv_obj_t* parent):root_(box(parent,geometry::TurnGuidanceLayout::bounds)){
    auto* root=root_.get();lv_obj_set_style_radius(root,5,0);lv_obj_set_style_border_width(root,1,0);
    source_=label(root,84,3,410,"FROM GOOGLE MAPS NOTIFICATION",fonts::caption);
    instruction_=label(root,84,19,410,"",fonts::value);lv_label_set_long_mode(instruction_,LV_LABEL_LONG_DOT);
    road_=label(root,84,41,410,"",fonts::label);lv_label_set_long_mode(road_,LV_LABEL_LONG_DOT);
    summary_=label(root,84,63,410,"",fonts::caption);lv_label_set_long_mode(summary_,LV_LABEL_LONG_DOT);
    distance_=label(root,4,64,76,"",fonts::label);lv_obj_set_style_text_align(distance_,LV_TEXT_ALIGN_CENTER,0);
    neutral_=label(root,24,19,42,"i",fonts::heading);exit_=label(root,24,28,24,"",fonts::caption);
    lv_obj_set_style_text_align(exit_,LV_TEXT_ALIGN_CENTER,0);
    path_=lv_line_create(root);head_=lv_line_create(root);
    for(auto* line:{path_,head_}){lv_obj_set_pos(line,8,9);lv_obj_set_style_line_width(line,4,0);lv_obj_set_style_line_rounded(line,true,0);}
    show(root,false);
}
void TurnGuidanceCard::update(const presentation::TurnCard& s,const Palette& p,bool visible,bool simulated){
    show(root_.get(),visible);if(!visible)return;
    text(source_,simulated?"SIMULATION - MAPS NOTIFICATION":"FROM GOOGLE MAPS NOTIFICATION");
    paint(root_.get(),p.panel);if(borderColor_!=p.line){lv_obj_set_style_border_color(root_.get(),color(p.line),0);borderColor_=p.line;}
    ink(source_,p.muted);ink(instruction_,s.emphasis?p.ink:p.muted);ink(road_,p.muted);ink(summary_,p.muted);ink(distance_,s.emphasis?p.accent:p.muted);ink(neutral_,p.muted);ink(exit_,p.accent);
    text(instruction_,s.instruction);text(road_,s.road);text(summary_,s.summary);text(distance_,s.distance);
    const lv_point_precise_t *path=straight,*head=straightHead;unsigned count=2,headCount=3;
    using guidance::Maneuver;
    switch(s.maneuver){
    case Maneuver::Left:path=left;head=leftHead;count=3;break;
    case Maneuver::Right:path=right;head=rightHead;count=3;break;
    case Maneuver::KeepLeft:path=keepLeft;head=keepLeftHead;count=3;break;
    case Maneuver::KeepRight:path=keepRight;head=keepRightHead;count=3;break;
    case Maneuver::Uturn:path=uturn;head=uturnHead;count=6;break;
    case Maneuver::Roundabout:path=roundabout;head=roundHead;count=10;break;
    case Maneuver::Arrive:path=arrive;head=arriveHead;count=5;headCount=2;break;
    default:break;
    }
    const bool arrow=s.emphasis&&s.maneuver!=Maneuver::Unknown;
    show(path_,arrow);show(head_,arrow);show(neutral_,!arrow);show(exit_,arrow&&s.exit);
    if(arrow){if(!glyphInitialized_||previous_!=s.maneuver){lv_line_set_points(path_,path,count);lv_line_set_points(head_,head,headCount);previous_=s.maneuver;glyphInitialized_=true;}if(lineColor_!=p.accent){for(auto* line:{path_,head_})lv_obj_set_style_line_color(line,color(p.accent),0);lineColor_=p.accent;}}
    if(s.exit){char number[12];std::snprintf(number,sizeof number,"%u",s.exit);text(exit_,number);}
}
}
