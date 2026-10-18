#pragma once
#include <cstdint>
#include "embedded/application/DashboardPages.h"
namespace cluster::ui {
struct Metrics {
    static constexpr int width=800, height=480, margin=28, content=744;
    static constexpr int headerRule=85, tellTaleY=98, rpmY=164;
    static constexpr int speedX=240, speedWidth=320, fuelX=583, fuelWidth=189;
    static constexpr int fuelSegmentCount=8, fuelSkew=1;
    static constexpr int fuelY(bool track) { return track?420:440; }
    static constexpr int shiftRpm=10500, shiftFlashRpm=11400;
    static constexpr unsigned shiftPeriod=300, fuelPeriod=500, turnPeriod=450;
};
struct Palette {
    std::uint32_t background, ink, muted, panel, line, accent, amber, red, beam;
};
inline constexpr Palette midnight{0x090e14,0xedf4fb,0x91a4b7,0x14202c,0x253544,0x78e4c1,0xf5d45b,0xff7980,0x77baff};
inline constexpr Palette daylight{0xeff3f5,0x122333,0x4d6577,0xe0e8ec,0xc7d3db,0x007757,0x946000,0xbf263a,0x246bb0};
struct ViewOptions { bool light=false, track=false, imperial=false, iconTest=false; TripPage tripPage=TripPage::Odometer; InfoPage infoPage=InfoPage::Coolant; const char* time="—:—"; const char* date="CLOCK UNAVAILABLE"; };
} // namespace cluster::ui
