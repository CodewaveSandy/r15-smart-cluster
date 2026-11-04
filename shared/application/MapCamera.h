#pragma once
#include "shared/model/MapScene.h"
#include "shared/model/MapProjection.h"
#include <algorithm>
#include <cmath>

namespace cluster::maps {
// Display-local application state. Metres in the active scene's local plane;
// never a GPS observation or a phone route request. No radio/LVGL dependencies.
struct CameraView {
    double east=0,north=0,pixelsPerMetre=0.48;
};
class MapCamera {
public:
    bool following()const{return follow_;}
    bool ready()const{return ready_;}
    CameraView view()const{return view_;}
    unsigned zoomLimit()const{return mercator_?6:3;}
    void follow(){follow_=true;area_=false;}
    void area(){follow_=false;area_=true;}
    void browse(){follow_=false;area_=false;}
    void update(const Scene* scene,const Pose& pose,unsigned zoom){
        zoom=std::clamp(zoom,1u,scene&&scene->mercator?6u:3u);
        if(scene!=bound_){
            if(scene&&ready_&&!follow_){
                constexpr double k=6371008.8*3.141592653589793/180/10000000;
                const double oldCos=std::cos(originLat_*3.141592653589793/180/10000000);
                const auto origin=mercator(originLat_,originLon_);
                const double lon=mercator_?longitudeE7(origin.east+view_.east):originLon_+view_.east/(k*oldCos);
                const double lat=mercator_?latitudeE7(origin.north+view_.north):originLat_+view_.north/k;
                const double newCos=std::cos(scene->originLatE7*3.141592653589793/180/10000000);
                const auto target=mercator(std::int32_t(lat),std::int32_t(lon)),base=mercator(scene->originLatE7,scene->originLonE7);
                view_.east=scene->mercator?wrapEast(target.east-base.east):std::clamp((lon-scene->originLonE7)*k*newCos,-20000.0,20000.0);
                view_.north=scene->mercator?target.north-base.north:std::clamp((lat-scene->originLatE7)*k,-20000.0,20000.0);
            }
            bound_=scene;if(scene){originLat_=scene->originLatE7;originLon_=scene->originLonE7;mercator_=scene->mercator;}
            if(follow_)ready_=false;
        }
        if(!ready_&&scene){fit(*scene);ready_=true;}
        if(area_&&scene){fit(*scene);area_=false;overview_=true;}
        if(follow_&&pose.quality==Quality::Available){view_.east=pose.point.east;view_.north=pose.point.north;ready_=true;overview_=false;}
        if(zoom!=zoom_){overview_=false;zoom_=zoom;}
        if(!overview_)view_.pixelsPerMetre=mercator_?0.12*std::pow(2.0,zoom-1):0.48*zoom;
    }
    // A positive screen delta moves the rendered roads with the finger. Clamp
    // to the v1 local-plane envelope; a future geographic model replaces this.
    void drag(int dx,int dy){
        browse();
        view_.east=mercator_?wrapEast(view_.east-dx/view_.pixelsPerMetre):std::clamp(view_.east-dx/view_.pixelsPerMetre,-20000.0,20000.0);
        const auto base=mercator(originLat_,originLon_);
        view_.north=mercator_?std::clamp(view_.north+dy/view_.pixelsPerMetre,-worldMetres/2-base.north,worldMetres/2-base.north):std::clamp(view_.north+dy/view_.pixelsPerMetre,-20000.0,20000.0);
        ready_=true;
    }
    void zoom(){overview_=false;area_=false;}
private:
    void fit(const Scene& s){
        int left=40075017,right=-40075017,bottom=40075017,top=-40075017;
        const auto add=[&](Point p){left=std::min(left,int(p.east));right=std::max(right,int(p.east));bottom=std::min(bottom,int(p.north));top=std::max(top,int(p.north));};
        if(s.overviewCount)for(unsigned i=0;i<s.overviewCount;++i)add(s.routeOverview[i]);
        else if(s.routeCount)for(unsigned i=0;i<s.routeCount;++i)add(s.route[i]);
        else for(unsigned i=0;i<s.roadCount;++i)for(unsigned j=0;j<s.roads[i].count;++j)add(s.roads[i].points[j]);
        if(right<left)return;
        view_={double(left+right)/2,double(top+bottom)/2,std::clamp(std::min(440.0/std::max(1,right-left),238.0/std::max(1,top-bottom)),s.mercator?0.000001:0.005,3.84)};
        overview_=true;
    }
    CameraView view_{};const Scene* bound_=nullptr;
    std::int32_t originLat_=0,originLon_=0;
    bool follow_=true,area_=false,ready_=false,overview_=false,mercator_=false;unsigned zoom_=1;
};
}
