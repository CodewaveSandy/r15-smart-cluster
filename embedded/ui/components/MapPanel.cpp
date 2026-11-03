#include "MapPanel.h"
#include "../WidgetPrimitives.h"
#include <algorithm>
#include <cmath>

namespace cluster::ui {
using namespace widget;
namespace {
struct Pixel { double x,y; };
struct Camera {
    double east,north,scale,cx,cy;
    Pixel project(maps::Point p) const { return {cx+(p.east-east)*scale,cy-(p.north-north)*scale}; }
};
// Clip before creating LVGL draw tasks. Off-screen world geometry cannot create
// large raster bounds; widths are clipped by LVGL's map-object clip region.
bool clip(Pixel& a,Pixel& b,double left,double top,double right,double bottom) {
    const double dx=b.x-a.x,dy=b.y-a.y;
    const double p[]={-dx,dx,-dy,dy},q[]={a.x-left,right-a.x,a.y-top,bottom-a.y};
    double lo=0,hi=1;
    for(unsigned i=0;i<4;++i) {
        if(p[i]==0) { if(q[i]<0)return false;continue; }
        const double t=q[i]/p[i];
        if(p[i]<0)lo=std::max(lo,t);else hi=std::min(hi,t);
        if(lo>hi)return false;
    }
    b={a.x+hi*dx,a.y+hi*dy};a={a.x+lo*dx,a.y+lo*dy};return true;
}
void line(lv_layer_t* layer,Pixel a,Pixel b,int width,std::uint32_t rgb,const lv_area_t& area) {
    if(!clip(a,b,area.x1,area.y1,area.x2,area.y2))return;
    lv_draw_line_dsc_t d;lv_draw_line_dsc_init(&d);
    d.p1={int(std::lround(a.x)),int(std::lround(a.y))};d.p2={int(std::lround(b.x)),int(std::lround(b.y))};
    d.width=width;d.color=color(rgb);d.round_start=1;d.round_end=1;
    lv_draw_line(layer,&d);
}
void dot(lv_layer_t* layer,Pixel center,int radius,std::uint32_t rgb){
    lv_draw_rect_dsc_t d;lv_draw_rect_dsc_init(&d);d.bg_color=color(rgb);d.bg_opa=255;d.radius=LV_RADIUS_CIRCLE;
    const int x=int(std::lround(center.x)),y=int(std::lround(center.y));const lv_area_t area{x-radius,y-radius,x+radius,y+radius};lv_draw_rect(layer,&d,&area);
}
void triangle(lv_layer_t* layer,Pixel a,Pixel b,Pixel c,std::uint32_t rgb) {
    lv_draw_triangle_dsc_t d;lv_draw_triangle_dsc_init(&d);
    d.p[0]={int(std::lround(a.x)),int(std::lround(a.y))};d.p[1]={int(std::lround(b.x)),int(std::lround(b.y))};d.p[2]={int(std::lround(c.x)),int(std::lround(c.y))};
    d.bg_color=color(rgb);d.bg_opa=255;lv_draw_triangle(layer,&d);
}
}
MapPanel::MapPanel(lv_obj_t* parent):root_(box(parent,MapGeometry::x,MapGeometry::y,MapGeometry::width,MapGeometry::height)) {
    lv_obj_add_event_cb(root_.get(),draw,LV_EVENT_DRAW_MAIN,this);
}
bool MapPanel::setScene(const maps::Scene& scene) {
    if(!maps::valid(scene))return false;
    if(!owned_)owned_=std::make_unique<maps::Scene>();
    *owned_=scene;bindScene(owned_.get());return true;
}
void MapPanel::visible(bool value){show(root_.get(),value);}
void MapPanel::bindScene(const maps::Scene* scene){if(scene_!=scene){scene_=scene;lv_obj_invalidate(root_.get());}}
void MapPanel::update(const maps::Pose& pose,bool overview,bool light,unsigned zoom,bool browsing) {
    auto next=pose;
    if(!maps::valid(next))next=maps::Pose{{},0,Quality::Invalid};
    zoom=std::clamp(zoom,1u,3u);
    if(initialized_&&pose_.point.east==next.point.east&&pose_.point.north==next.point.north&&
        pose_.headingDegrees==next.headingDegrees&&pose_.directionKnown==next.directionKnown&&pose_.quality==next.quality&&overview_==overview&&light_==light&&zoom_==zoom&&browsing_==browsing)return;
    initialized_=true;pose_=next;overview_=overview;light_=light;zoom_=zoom;browsing_=browsing;
    paint(root_.get(),(light?mapDay:mapNight).ground);lv_obj_invalidate(root_.get());
}
void MapPanel::draw(lv_event_t* e) {
    const auto& self=*static_cast<const MapPanel*>(lv_event_get_user_data(e));
    if(!self.scene_||self.scene_->quality!=Quality::Available)return;
    auto* layer=lv_event_get_layer(e);lv_area_t area;lv_obj_get_coords(self.root_.get(),&area);
    const auto& style=self.light_?mapDay:mapNight;
    Camera camera{double(self.pose_.point.east),double(self.pose_.point.north),0.48*self.zoom_,double(area.x1+495),double(area.y1+212)};
    if(self.overview_||(!self.browsing_&&self.pose_.quality!=Quality::Available)) {
        int left=20000,right=-20000,bottom=20000,top=-20000;
        for(unsigned i=0;i<self.scene_->routeCount;++i) {
            const auto p=self.scene_->route[i];left=std::min(left,int(p.east));right=std::max(right,int(p.east));
            bottom=std::min(bottom,int(p.north));top=std::max(top,int(p.north));
        }
        if(self.scene_->routeCount==0)for(unsigned i=0;i<self.scene_->roadCount;++i)for(unsigned j=0;j<self.scene_->roads[i].count;++j) {
            const auto p=self.scene_->roads[i].points[j];left=std::min(left,int(p.east));right=std::max(right,int(p.east));
            bottom=std::min(bottom,int(p.north));top=std::max(top,int(p.north));
        }
        camera={double(left+right)/2,double(top+bottom)/2,
            std::min(440.0/std::max(1,right-left),238.0/std::max(1,top-bottom)),double(area.x1+525),double(area.y1+150)};
    }
    // Roads are painted before route/marker; only bounded in-memory primitives.
    for(unsigned pass=0;pass<2;++pass)for(unsigned i=0;i<self.scene_->roadCount;++i) {
        const auto& road=self.scene_->roads[i];
        if(camera.scale<0.2&&(road.kind==maps::RoadKind::Path||road.kind==maps::RoadKind::Service))continue;
        const double factor=std::clamp(camera.scale/0.48,0.1,1.5);
        const int base=road.kind==maps::RoadKind::Water?30:road.kind==maps::RoadKind::Arterial?12:road.kind==maps::RoadKind::Path?2:road.kind==maps::RoadKind::Service?3:6;
        const int width=std::max(1,int(std::lround(base*factor)));
        const auto rgb=road.kind==maps::RoadKind::Water?style.water:pass==0?style.roadEdge:style.road;
        for(unsigned j=1;j<road.count;++j)line(layer,camera.project(road.points[j-1]),camera.project(road.points[j]),width+(pass==0?std::max(1,int(std::lround(4*factor))):0),rgb,area);
    }
    for(unsigned pass=0;pass<2;++pass)for(unsigned i=1;i<self.scene_->routeCount;++i)
        line(layer,camera.project(self.scene_->route[i-1]),camera.project(self.scene_->route[i]),pass==0?14:8,pass==0?style.routeEdge:style.route,area);
    unsigned labels=0;
    std::array<lv_area_t,8> labelBounds{};
    std::array<const char*,8> labelNames{};
    for(unsigned priority=0;priority<2;++priority)for(unsigned i=0;i<self.scene_->roadCount&&labels<8;++i) {
        const auto& road=self.scene_->roads[i];if(!road.name[0])continue;
        if(unsigned(road.kind==maps::RoadKind::Arterial?0:1)!=priority)continue;
        if(camera.scale<0.2&&road.kind!=maps::RoadKind::Arterial)continue;
        bool duplicate=false;
        for(unsigned j=0;j<labels;++j)if(std::strcmp(labelNames[j],road.name.data())==0)duplicate=true;
        if(duplicate)continue;
        const auto p=camera.project(road.points[road.count/2]);
        if(p.x<area.x1+285||p.x>area.x2-175||p.y<area.y1+22||p.y>area.y2-50)continue;
        lv_area_t bounds{int(p.x+10),int(p.y+10),int(p.x+174),int(p.y+38)};
        bool overlaps=false;
        for(unsigned j=0;j<labels;++j) {
            const auto& other=labelBounds[j];
            if(bounds.x1<=other.x2+10&&bounds.x2>=other.x1-10&&bounds.y1<=other.y2+10&&bounds.y2>=other.y1-10)overlaps=true;
        }
        if(overlaps)continue;
        lv_draw_label_dsc_t d;lv_draw_label_dsc_init(&d);d.font=fonts::caption;d.color=color(style.label);d.text=road.name.data();
        lv_draw_label(layer,&d,&bounds);labelBounds[labels]=bounds;labelNames[labels]=road.name.data();++labels;
    }
    if(self.scene_->routeCount>0) {
    const auto destination=camera.project(self.scene_->route[self.scene_->routeCount-1]);
    if(destination.x>=area.x1+8&&destination.x<=area.x2-8&&destination.y>=area.y1+8&&destination.y<=area.y2-8) {
        line(layer,{destination.x-6,destination.y},{destination.x+6,destination.y},11,style.route,area);
        line(layer,{destination.x,destination.y-6},{destination.x,destination.y+6},3,style.marker,area);
    }
    }
    if(!self.browsing_&&self.pose_.quality==Quality::Available) {
        const auto center=camera.project(self.pose_.point);
        if(center.x<area.x1+20||center.x>area.x2-20||center.y<area.y1+20||center.y>area.y2-20)return;
        const double a=self.pose_.headingDegrees*3.141592653589793/180;
        const auto point=[&](double x,double y){return Pixel{center.x+x*std::cos(a)+y*std::sin(a),center.y+x*std::sin(a)-y*std::cos(a)};};
        if(!self.pose_.directionKnown){
            dot(layer,center,12,style.route);dot(layer,center,7,style.marker);
        }else{
        triangle(layer,point(0,23),point(-18,-14),point(18,-14),style.route);
        triangle(layer,point(0,18),point(-12,-10),point(12,-10),style.marker);
        }
    }
    if(!self.overview_&&(self.browsing_||self.pose_.quality==Quality::Available)) {
        const double y=area.y2-9,left=area.x2-119,right=area.x2-23;
        line(layer,{left,y},{right,y},2,style.label,area);
        line(layer,{left,y-4},{left,y+2},2,style.label,area);
        line(layer,{right,y-4},{right,y+2},2,style.label,area);
    }
}
} // namespace cluster::ui
