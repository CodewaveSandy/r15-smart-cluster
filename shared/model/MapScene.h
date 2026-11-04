#pragma once
#include "Quality.h"
#include <array>
#include <cstdint>
#include <cstring>

namespace cluster::maps {
// Bounded local-plane rendering input, not a geographic/wire protocol. The
// producer projects coordinates into metres; this layer knows no LVGL or radio.
struct Point { std::int32_t east=0,north=0; };
enum class RoadKind { Local, Arterial, Water, Path, Service };
struct Road {
    std::array<Point,8> points{};
    unsigned count=0;
    RoadKind kind=RoadKind::Local;
    std::array<char,24> name{};
};
struct Scene {
    // Local-plane origin for camera rebasing across validated regional swaps.
    // Zero is also a valid synthetic/host origin; wire producers set E7 values.
    std::int32_t originLatE7=0,originLonE7=0;
    bool mercator=false;
    // Shared rendering ceiling. Firmware advertises readiness only after PSRAM
    // reservation; capacity alone is not measured board/rendering performance.
    std::array<Road,2048> roads{};
    unsigned roadCount=0;
    std::array<Point,512> route{},routeOverview{};
    unsigned routeCount=0;
    unsigned overviewCount=0;
    Quality quality=Quality::Unavailable;
};
enum class Marker {Normal,Approximate,LastKnown};
struct Pose { Point point{}; unsigned headingDegrees=0; Quality quality=Quality::Unavailable; bool directionKnown=true; Marker marker=Marker::Normal;unsigned accuracyMetres=0; };
inline bool valid(Point p) { return p.east>=-40075017&&p.east<=40075017&&p.north>=-40075017&&p.north<=40075017; }
inline bool valid(const Scene& s) {
    const int limit=s.mercator?850511287:850000000;
    if(s.originLatE7< -limit||s.originLatE7>limit||s.originLonE7< -1800000000||s.originLonE7>1800000000)return false;
    if(s.roadCount>s.roads.size()||s.routeCount>s.route.size()||s.overviewCount>s.routeOverview.size()||unsigned(s.quality)>unsigned(Quality::Stale))return false;
    if(s.quality==Quality::Available&&((s.roadCount==0&&!s.mercator)||s.routeCount==1||s.overviewCount==1))return false;
    for(unsigned i=0;i<s.roadCount;++i) {
        const auto& r=s.roads[i];
        if(r.count<2||r.count>r.points.size()||unsigned(r.kind)>unsigned(RoadKind::Service)||!std::memchr(r.name.data(),0,r.name.size()))return false;
        for(char c:r.name){if(c==0)break;if(c<32||c>126)return false;}
        for(unsigned j=0;j<r.count;++j)if(!valid(r.points[j]))return false;
    }
    for(unsigned i=0;i<s.routeCount;++i)if(!valid(s.route[i]))return false;
    for(unsigned i=0;i<s.overviewCount;++i)if(!valid(s.routeOverview[i]))return false;
    return true;
}
inline bool valid(const Pose& p) { return valid(p.point)&&p.headingDegrees<360&&unsigned(p.quality)<=unsigned(Quality::Stale); }
} // namespace cluster::maps
