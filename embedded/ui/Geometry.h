#pragma once
#include "Tokens.h"
#include <array>

namespace cluster::ui::geometry {
// Label height comes from its font. All positions are logical 800 x 480 pixels.
struct TextArea { int x,y,width; };
struct Rect { int x,y,width,height; };
struct Point { int x,y; };
struct HeaderLayout {
    static constexpr Rect bounds{0,0,Metrics::width,Metrics::headerRule};
    static constexpr TextArea gear{28,16,65}, mode{120,26,160}, subtitle{121,56,200};
    static constexpr TextArea phoneBattery{508,36,100}, qs{288,36,48}, vva{344,36,60};
    static constexpr TextArea clock{650,22,122}, date{630,57,142};
    static constexpr Point phone{456,31};
    static constexpr Rect divider{Metrics::margin,Metrics::headerRule,Metrics::content,1};
    static constexpr Rect gearDivider{96,22,1,48};
};
struct TellTaleLayout {
    static constexpr std::array<int,11> x{{32,100,171,242,313,384,455,526,597,668,736}};
    static constexpr int y=Metrics::tellTaleY;
    static constexpr TextArea neutral{x[1],101,32};
};
struct TachometerLayout {
    static constexpr int tickCount=14, scaleMaxRpm=13000;
    static constexpr TextArea tick(int i) { return {Metrics::margin+i*Metrics::content/(tickCount-1),144,30}; }
    static constexpr Rect bed{Metrics::margin,Metrics::rpmY,Metrics::content,Metrics::rpmHeight};
    static constexpr Rect clip{0,0,1,Metrics::rpmHeight};
    static constexpr Rect ramp{0,0,Metrics::content,Metrics::rpmHeight};
    static constexpr TextArea caption{28,220,160}, value{610,220,114}, unit{730,220,42};
};
struct ReadoutLayout {
    static constexpr TextArea streetSpeed{Metrics::speedX,240,Metrics::speedWidth};
    static constexpr TextArea streetUnit{560,310,90};
    static constexpr TextArea trackSpeed{560,269,212}, trackUnit{715,295,57};
    static constexpr TextArea lap{Metrics::speedX,Metrics::trackLapY,Metrics::speedWidth};
    static constexpr TextArea lapCaption{Metrics::speedX,Metrics::trackLapTitleY,Metrics::speedWidth};
    static constexpr TextArea speedCaption{560,247,212};
};
struct RideInfoLayout {
    static constexpr Rect divider{Metrics::margin,Metrics::lowerDividerY,Metrics::content,1};
    static constexpr TextArea tripTitle{28,Metrics::lowerTitleY,220};
    static constexpr TextArea tripValue{28,Metrics::lowerTitleY+22,170};
    static constexpr TextArea tripUnit{203,Metrics::lowerTitleY+32,55};
    static constexpr TextArea infoTitle{267,Metrics::lowerTitleY,260};
    static constexpr TextArea infoValue{267,Metrics::lowerTitleY+22,265};
};
struct FuelLayout {
    static constexpr Rect divider{537,Metrics::lowerTitleY,1,55};
    static constexpr TextArea title{Metrics::fuelX,Metrics::lowerTitleY,110};
    static constexpr TextArea value{690,Metrics::lowerTitleY-5,82};
    static constexpr Point pump{550,Metrics::fuelY-10};
    static constexpr Rect reserveMarker{Metrics::fuelX+Metrics::fuelWidth*presentation::Policy::fuelReservePercent/100,Metrics::fuelY-5,2,4};
    static constexpr Rect segment(int i) {
        const int x=Metrics::fuelX+i*Metrics::fuelWidth/Metrics::fuelSegmentCount;
        const int end=Metrics::fuelX+(i+1)*Metrics::fuelWidth/Metrics::fuelSegmentCount;
        return {x,Metrics::fuelY,end-x,12};
    }
};
struct LifecycleLayout {
    static constexpr Rect bounds{0,0,Metrics::width,Metrics::height};
    static constexpr Point center{Metrics::width/2,220};
    static constexpr Rect wordmark{304,220,192,64};
    static constexpr int highlightY=286;
    static constexpr Point outer{-286,-48},elbow{-170,20},inner{-92,44};
};
} // namespace cluster::ui::geometry
