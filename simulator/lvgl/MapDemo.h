#pragma once
#include "shared/model/MapScene.h"
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <initializer_list>

namespace cluster::host {
// Fictional local-metre grid, authored for rendering. Not OSM, a real road
// network, GPS, a routing engine or a map-provider/projection implementation.
inline maps::Scene mapDemoScene() {
    maps::Scene s;s.quality=Quality::Available;
    const auto road=[&](maps::RoadKind kind,const char* name,std::initializer_list<maps::Point> points) {
        auto& r=s.roads[s.roadCount++];r.kind=kind;r.count=unsigned(points.size());
        std::copy(points.begin(),points.end(),r.points.begin());std::snprintf(r.name.data(),r.name.size(),"%s",name);
    };
    using Kind=maps::RoadKind;
    road(Kind::Water,"Canal",{{-500,1500},{300,1500},{900,1650},{1700,1550},{3000,1650}});
    for(int y:{-200,200,700,1000,1350,1800,2100,2500})
        road(y==700||y==1800?Kind::Arterial:Kind::Local,y==700?"Market Road":y==1350?"Canal Street":y==1800?"Garden Road":"",{{-500,y},{3000,y}});
    for(int x:{-100,300,550,1050,1550,1900,2150,2500})
        road(x==1050?Kind::Arterial:Kind::Local,x==1050?"Lake Avenue":x==1550?"Park Street":"",{{x,-500},{x,2800}});
    road(Kind::Arterial,"Ring Road",{{-400,-200},{200,400},{950,1000},{1750,1500},{2700,2300}});
    road(Kind::Local,"",{{-300,2250},{650,2000},{1250,2100},{2400,2450}});
    road(Kind::Local,"",{{100,-400},{850,150},{1500,350},{2650,500}});
    const maps::Point route[]={{550,200},{550,700},{1050,700},{1050,1350},{1550,1350},{1550,1800},{2150,1800}};
    s.routeCount=unsigned(std::size(route));std::copy(std::begin(route),std::end(route),s.route.begin());
    return s;
}
struct MapDemoFrame {
    maps::Pose pose;
    unsigned segment=0,turnMeters=0,remainingMeters=0,etaMinutes=0;
    bool arrived=false;
};
inline MapDemoFrame mapDemoFrame(const maps::Scene& s,std::uint64_t elapsed) {
    MapDemoFrame frame;frame.pose.quality=Quality::Available;
    double total=0;
    for(unsigned i=1;i<s.routeCount;++i)total+=std::hypot(double(s.route[i].east-s.route[i-1].east),double(s.route[i].north-s.route[i-1].north));
    const double travelled=std::min(total,elapsed*0.022); // Explicit sample animation, 22 metres/second.
    frame.remainingMeters=unsigned(std::ceil(total-travelled));frame.etaMinutes=(frame.remainingMeters+1319)/1320;
    double remaining=travelled;
    for(unsigned i=1;i<s.routeCount;++i) {
        const auto a=s.route[i-1],b=s.route[i];const double length=std::hypot(double(b.east-a.east),double(b.north-a.north));
        if(remaining<=length||i+1==s.routeCount) {
            const double t=length>0?std::clamp(remaining/length,0.0,1.0):0;
            frame.pose.point={int(std::lround(a.east+(b.east-a.east)*t)),int(std::lround(a.north+(b.north-a.north)*t))};
            const double degrees=std::atan2(double(b.east-a.east),double(b.north-a.north))*180/3.141592653589793;
            frame.pose.headingDegrees=unsigned(int(std::lround(degrees))+360)%360;
            frame.segment=i-1;frame.turnMeters=unsigned(std::ceil(std::max(0.0,length-remaining)));break;
        }
        remaining-=length;
    }
    frame.arrived=travelled>=total;return frame;
}
} // namespace cluster::host
