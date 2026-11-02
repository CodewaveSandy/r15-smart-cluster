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
    // Host real-area prototype ceiling. This capacity is not a firmware memory
    // budget; board delivery will need measured visible-area chunks/caching.
    std::array<Road,2048> roads{};
    unsigned roadCount=0;
    std::array<Point,32> route{};
    unsigned routeCount=0;
    Quality quality=Quality::Unavailable;
};
struct Pose { Point point{}; unsigned headingDegrees=0; Quality quality=Quality::Unavailable; };
inline bool valid(Point p) { return p.east>=-20000&&p.east<=20000&&p.north>=-20000&&p.north<=20000; }
inline bool valid(const Scene& s) {
    if(s.roadCount>s.roads.size()||s.routeCount>s.route.size()||unsigned(s.quality)>unsigned(Quality::Stale))return false;
    if(s.quality==Quality::Available&&(s.roadCount==0||s.routeCount==1))return false;
    for(unsigned i=0;i<s.roadCount;++i) {
        const auto& r=s.roads[i];
        if(r.count<2||r.count>r.points.size()||unsigned(r.kind)>unsigned(RoadKind::Service)||!std::memchr(r.name.data(),0,r.name.size()))return false;
        for(char c:r.name){if(c==0)break;if(c<32||c>126)return false;}
        for(unsigned j=0;j<r.count;++j)if(!valid(r.points[j]))return false;
    }
    for(unsigned i=0;i<s.routeCount;++i)if(!valid(s.route[i]))return false;
    return true;
}
inline bool valid(const Pose& p) { return valid(p.point)&&p.headingDegrees<360&&unsigned(p.quality)<=unsigned(Quality::Stale); }
} // namespace cluster::maps
