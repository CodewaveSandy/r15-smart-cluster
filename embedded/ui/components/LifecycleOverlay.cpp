#include "LifecycleOverlay.h"
#include "../WidgetPrimitives.h"
#include <algorithm>

extern "C" { LV_IMAGE_DECLARE(r15_signature); }
namespace cluster::ui {
using namespace widget;
using namespace presentation;
namespace {
using Point=geometry::Point;
void line(lv_layer_t* layer,Point a,Point b,int width,std::uint32_t rgb,unsigned opacity) {
    if(!opacity) return;
    lv_draw_line_dsc_t d; lv_draw_line_dsc_init(&d);
    d.p1={a.x,a.y}; d.p2={b.x,b.y}; d.width=width; d.color=color(rgb); d.opa=opacity;
    lv_draw_line(layer,&d);
}
void triangle(lv_layer_t* layer,Point a,Point b,Point c,std::uint32_t rgb,unsigned opacity) {
    if(!opacity) return;
    lv_draw_triangle_dsc_t d; lv_draw_triangle_dsc_init(&d);
    d.p[0]={a.x,a.y}; d.p[1]={b.x,b.y}; d.p[2]={c.x,c.y};
    d.bg_color=color(rgb); d.bg_opa=opacity;
    lv_draw_triangle(layer,&d);
}
Point interpolate(Point a,Point b,int progress) {
    return {a.x+(b.x-a.x)*progress/1000,a.y+(b.y-a.y)*progress/1000};
}
void blade(lv_layer_t* layer,Point outer,Point elbow,Point inner,int width,int trim,std::uint32_t rgb,unsigned opacity) {
    if(trim>=1000||!opacity) return;
    const int direction=inner.x>elbow.x?1:-1;
    if(trim<650) {
        const auto start=interpolate(outer,elbow,trim*1000/650);
        const int slant=std::max(1,width/2)*direction;
        triangle(layer,start,{elbow.x+slant,elbow.y-width},{elbow.x-slant,elbow.y+width},rgb,opacity);
    }
    const int progress=std::max(0,trim-650)*1000/350;
    const auto start=interpolate(elbow,inner,progress);
    const int remaining=std::max(1,width*(1000-progress)/1000);
    const int slant=std::max(1,remaining/2)*direction;
    triangle(layer,{start.x+slant,start.y-remaining},{start.x-slant,start.y+remaining},inner,rgb,opacity);
}
bool same(const AnimationFrame& a,const AnimationFrame& b) {
    return a.stage==b.stage&&a.overlay==b.overlay&&a.dashboardVisible==b.dashboardVisible&&
        a.cover==b.cover&&a.logo==b.logo&&a.lights==b.lights&&a.highlight==b.highlight&&
        a.blade==b.blade&&a.trim==b.trim&&a.logoScale==b.logoScale&&
        a.highlightPosition==b.highlightPosition&&a.separation==b.separation;
}
}
LifecycleOverlay::LifecycleOverlay(lv_obj_t* parent) {
    overlay_.reset(box(parent,geometry::LifecycleLayout::bounds));
    paint(overlay_.get(),0x000000);
    lv_obj_add_event_cb(overlay_.get(),draw,LV_EVENT_DRAW_MAIN,this);
    wordmark_=lv_image_create(overlay_.get());
    lv_image_set_src(wordmark_,&r15_signature);
    const auto bounds=geometry::LifecycleLayout::wordmark;
    lv_obj_set_pos(wordmark_,bounds.x,bounds.y);
    lv_image_set_pivot(wordmark_,bounds.width/2,bounds.height/2);
    lv_obj_set_style_image_recolor(wordmark_,color(0xffffff),0);
    lv_obj_set_style_image_recolor_opa(wordmark_,255,0);
}
void LifecycleOverlay::update(const AnimationFrame& next) {
    show(overlay_.get(),next.overlay);
    if(initialized_&&same(frame_,next)) return;
    frame_=next; initialized_=true;
    if(!frame_.overlay) return;
    lv_obj_set_style_bg_opa(overlay_.get(),frame_.cover,0);
    show(wordmark_,frame_.logo>0);
    if(frame_.logo) {
        lv_obj_set_style_image_opa(wordmark_,frame_.logo,0);
        lv_image_set_scale(wordmark_,frame_.logoScale);
    }
    lv_obj_invalidate(overlay_.get());
}
void LifecycleOverlay::draw(lv_event_t* event) {
    const auto* self=static_cast<const LifecycleOverlay*>(lv_event_get_user_data(event));
    const auto& f=self->frame_;
    auto* layer=lv_event_get_layer(event);
    lv_area_t area; lv_obj_get_coords(self->overlay_.get(),&area);
    const auto center=geometry::LifecycleLayout::center;
    const int cx=area.x1+center.x,cy=area.y1+center.y;
    const int width=1+6*f.blade/1000;
    // Two mirrored tapered blades: constant primitive count, no textures,
    // particle systems, animated allocations or full-screen flashes.
    for(int mirror:{1,-1}) {
        const auto point=[&](Point local) { return Point{cx+mirror*(local.x-f.separation),cy+local.y}; };
        const auto outer=point(geometry::LifecycleLayout::outer);
        const auto elbow=point(geometry::LifecycleLayout::elbow);
        const auto inner=point(geometry::LifecycleLayout::inner);
        if(f.lights&&f.trim<1000) {
            if(f.trim<650) {
                const auto start=interpolate(outer,elbow,f.trim*1000/650);
                line(layer,start,elbow,width*3,0x004de0,f.lights/12);
                line(layer,start,elbow,width*2,0x087aff,f.lights/7);
            }
            const auto start=interpolate(elbow,inner,std::max(0,int(f.trim)-650)*1000/350);
            line(layer,start,inner,width*3,0x004de0,f.lights/12);
            line(layer,start,inner,width*2,0x087aff,f.lights/7);
            blade(layer,outer,elbow,inner,width,f.trim,0x087aff,f.lights);
            blade(layer,outer,{elbow.x,elbow.y+1},inner,std::max(1,width/3),f.trim,0xd8f4ff,f.lights);
        }
    }
    if(f.logo) {
        const int half=32*f.logoScale/256;
        const int y=area.y1+geometry::LifecycleLayout::highlightY;
        line(layer,{cx-half,y},{cx+half,y},1,0x39739e,f.logo/3);
        if(f.highlight) {
            const int x=cx-32+64*f.highlightPosition/1000;
            line(layer,{x-7,y},{x+7,y},2,0xe6f8ff,f.highlight);
        }
    }
}
} // namespace cluster::ui
