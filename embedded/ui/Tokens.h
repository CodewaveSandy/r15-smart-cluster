#pragma once
#include <cstdint>
#include "embedded/application/DisplayOptions.h"
#include "embedded/presentation/PresentationPolicy.h"
namespace cluster::ui {
struct Metrics {
    static constexpr unsigned speedPlaceholderOpacity=40; // 16% leading-zero ink.
    static constexpr std::uint32_t shiftTop=0xa51c30, shiftMiddle=0xf04444, shiftBottom=0xc52636;
    static constexpr int width=800, height=480, margin=28, content=744;
    static constexpr int headerRule=85, tellTaleY=98, rpmY=164;
    static constexpr int speedX=240, speedWidth=320, fuelX=583, fuelWidth=189;
    static constexpr int fuelSegmentCount=presentation::Policy::fuelSegments, fuelSkew=4;
    static constexpr int rpmHeight=44, lowerDividerY=376, lowerTitleY=399, fuelY=440;
    static constexpr int trackLapY=259, trackLapTitleY=235;
    static constexpr unsigned turnPeriod=presentation::Policy::turnPeriod;
};
struct Palette {
    std::uint32_t background, ink, muted, panel, line, accent, amber, red, beam;
    std::uint32_t rpmBed, rpmGreen, rpmAmber, rpmRed;
};
inline constexpr Palette midnight{0x090e14,0xffffff,0x91a4b7,0x14202c,0x253544,0x78e4c1,0xf5d45b,0xff7980,0x77baff,
    0x14202c,0x4cdb88,0xf5d45b,0xf06464};
inline constexpr Palette daylight{0xeff3f5,0x000000,0x4d6577,0xe0e8ec,0xc7d3db,0x007757,0x946000,0xbf263a,0x246bb0,
    0xc2cfd6,0x00774b,0x946000,0xbf263a};
struct ViewOptions:DisplayOptions { bool iconTest=false, bluetoothDemo=false; const char* time="—:—"; const char* date="CLOCK UNAVAILABLE"; };
} // namespace cluster::ui
