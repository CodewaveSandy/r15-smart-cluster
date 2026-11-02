#include "MapDemo.h"
#include "MapFile.h"
#include "embedded/ui/components/MapPanel.h"
#include "embedded/ui/components/Header.h"
#include "embedded/ui/components/TellTaleRow.h"
#include "embedded/ui/components/PrimaryReadout.h"
#include "embedded/ui/components/LifecycleOverlay.h"
#include "embedded/ui/WidgetPrimitives.h"
#include "embedded/presentation/LightSignatureAnimation.h"
#include <SDL.h>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <memory>
#include <stdexcept>
#include <string>
#include <thread>
extern "C" { LV_FONT_DECLARE(inter_map_copyright_16); }

namespace {
using namespace cluster;
using namespace cluster::ui;
using namespace cluster::ui::widget;
std::array<std::uint16_t,800*480> pixels{};
bool dirty=true;
unsigned warnings=0;
std::uint64_t flushes=0;
void logMessage(lv_log_level_t,const char* value) { ++warnings;std::fprintf(stderr,"%s",value); }
void flush(lv_display_t* display,const lv_area_t* a,std::uint8_t* bytes) {
    const auto* source=reinterpret_cast<const std::uint16_t*>(bytes);
    for(int y=a->y1;y<=a->y2;++y)std::copy_n(source+(y-a->y1)*(a->x2-a->x1+1),a->x2-a->x1+1,pixels.data()+y*800+a->x1);
    dirty=true;++flushes;lv_display_flush_ready(display);
}
void capture(const std::filesystem::path& path) {
    lv_refr_now(nullptr);
    auto* surface=SDL_CreateRGBSurfaceWithFormatFrom(pixels.data(),800,480,16,1600,SDL_PIXELFORMAT_RGB565);
    if(!surface)throw std::runtime_error(SDL_GetError());
    const int result=SDL_SaveBMP(surface,path.string().c_str());SDL_FreeSurface(surface);
    if(result)throw std::runtime_error(SDL_GetError());
}
// Preview composition only. Firmware does not link this entry point or demo.
class MapScreen {
public:
    explicit MapScreen(bool realMap=false):root_(box(lv_screen_active(),0,0,800,480)),header_(root_.get()),tellTales_(root_.get()),
        primary_(root_.get()),map_(root_.get()),realMap_(realMap) {
        card_=box(root_.get(),20,160,244,112);lv_obj_set_style_radius(card_,10,0);
        direction_=label(card_,16,12,214,"",fonts::heading);
        maneuver_=label(card_,16,47,214,"",fonts::value);
        road_=label(card_,16,76,214,"",fonts::label);
        badge_=box(root_.get(),20,284,244,54);lv_obj_set_style_radius(badge_,8,0);
        badgeTitle_=label(badge_,12,8,222,"SYNTHETIC MAP",fonts::caption);
        badgeDetail_=label(badge_,12,29,222,"Sample route · no GPS",fonts::caption);
        north_=label(root_.get(),758,152,22,"N",fonts::value);
        scale_=label(root_.get(),680,414,100,"",fonts::caption);
        footer_=box(root_.get(),0,442,800,38);
        summary_=label(footer_,20,10,440,"",fonts::value);
        copyright_=label(footer_,20,10,18,"©",&inter_map_copyright_16);show(copyright_,realMap_);
        if(realMap_)lv_obj_set_x(summary_,40);
        mode_=label(footer_,470,12,310,"",fonts::caption);alignRight(mode_);
        notice_=box(root_.get(),310,258,440,64);lv_obj_set_style_radius(notice_,8,0);
        noticeText_=label(notice_,18,21,404,"",fonts::value);
        attribution_=label(root_.get(),20,416,475,"https://www.openstreetmap.org/copyright · ODbL",fonts::caption);
        show(attribution_,realMap_);
        overlay_=std::make_unique<LifecycleOverlay>(root_.get());
    }
    void update(const maps::Scene& scene,const host::MapDemoFrame& frame,ViewOptions view,
        const Lifecycle& life,std::uint64_t now,bool overview,bool paused,bool stale,bool missing,bool warning,unsigned zoom) {
        if(!loaded_||missing!=missing_) {
            auto input=scene;if(missing)input.quality=Quality::Unavailable;
            if(!map_.setScene(input))throw std::runtime_error("Invalid map scene");
            loaded_=true;missing_=missing;
        }
        auto p=view.light?cluster::ui::daylight:midnight;
        paint(root_.get(),p.background);paint(footer_,p.panel);paint(card_,p.panel);paint(badge_,p.panel);paint(notice_,p.panel);
        ClusterState vehicle;
        // Bike values remain unavailable: the sample route is not a bike sensor.
        vehicle.absWarning=vehicle.engineWarning=vehicle.coolantWarning={false,Quality::Available};
        vehicle.left=vehicle.right=vehicle.highBeam={false,Quality::Available};
        if(warning){vehicle.engineWarning.value=true;vehicle.coolantWarning.value=true;}
        view.simulated=true;view.time="—:—";view.date=realMap_?"KOLKATA OSM":"MAP RENDER PREVIEW";
        header_.update(vehicle,view,p,1000);tellTales_.update(vehicle,p,now,false);primary_.update(vehicle,view,p,1000);
        auto pose=frame.pose;if(stale)pose.quality=Quality::Stale;if(missing)pose.quality=Quality::Unavailable;
        map_.update(pose,overview,view.light,zoom,realMap_);
        if(!realMap_) {
        char turn[48],summary[96];
        std::snprintf(turn,sizeof turn,frame.arrived?"SAMPLE ARRIVAL":"%u m",frame.turnMeters);
        std::snprintf(summary,sizeof summary,"SAMPLE  %.1f km left  ·  %u min",frame.remainingMeters/1000.0,frame.etaMinutes);
        text(direction_,missing||stale?"GUIDANCE PAUSED":turn);
        const char* streets[]={"Market Road","Lake Avenue","Canal Street","Park Street","Garden Road","Sample destination"};
        text(maneuver_,missing?"Map unavailable":stale?"Position is stale":frame.arrived?"Destination reached":frame.segment==5?"Continue to destination":frame.segment%2==0?"Turn right":"Turn left");
        text(road_,missing||stale?"No live navigation":streets[std::min(frame.segment,5u)]);
        text(summary_,missing||stale?"SAMPLE ROUTE  ·  Guidance unavailable":summary);
        text(mode_,missing?"MAP UNAVAILABLE":stale?"OVERVIEW  ·  POSITION STALE":overview?"OVERVIEW  ·  NORTH UP":paused?"FOLLOW  ·  PAUSED":zoom==1?"FOLLOW  ·  NORTH UP":"FOLLOW  ·  ZOOMED");
        text(scale_,missing?"":overview||stale?"Route overview":zoom==1?"200 m":zoom==2?"100 m":"~67 m");
        text(noticeText_,missing?"Map unavailable · sample scene hidden":"Position stale · marker hidden");
        show(notice_,missing||stale);
        } else {
            text(direction_,"KOLKATA");
            text(maneuver_,missing?"Map unavailable":"West Bengal, India");
            text(road_,"Real OSM roads");
            text(badgeTitle_,"OFFLINE ROAD SNAPSHOT");text(badgeDetail_,"No route · no GPS");
            text(summary_,"OpenStreetMap contributors");
            text(mode_,missing?"MAP UNAVAILABLE":overview?"AREA OVERVIEW  ·  NO GPS":"BROWSE  ·  NO GPS");
            text(scale_,missing?"":overview?"Area overview":zoom==1?"200 m":zoom==2?"100 m":"~67 m");
            text(noticeText_,"Map unavailable · snapshot hidden");show(notice_,missing);
        }
        for(auto* o:{maneuver_,summary_})ink(o,p.ink);
        ink(direction_,stale||missing?p.amber:p.beam);ink(road_,p.muted);
        for(auto* o:{badgeTitle_,badgeDetail_,north_,scale_,mode_})ink(o,p.muted);
        ink(noticeText_,p.amber);
        ink(attribution_,p.muted);
        ink(copyright_,p.ink);
        if(realMap_)paint(attribution_,p.background);
        overlay_->update(presentation::animationFrame(life,warning,now));
    }
private:
    ObjectOwner root_;
    Header header_;TellTaleRow tellTales_;PrimaryReadout primary_;MapPanel map_;
    lv_obj_t *card_,*direction_,*maneuver_,*road_,*badge_,*badgeTitle_,*badgeDetail_,*north_,*scale_,*footer_,*summary_,*mode_,*notice_,*noticeText_;
    lv_obj_t *attribution_,*copyright_;
    std::unique_ptr<LifecycleOverlay> overlay_;
    bool loaded_=false,missing_=false;
    bool realMap_=false;
};
}
int main(int argc,char** argv) {
    try {
        bool render=false;std::filesystem::path output,mapFile;
        for(int i=1;i<argc;++i) {
            const std::string arg=argv[i];
            if((arg=="--render"||arg=="--map-file")&&i+1<argc) {
                auto& path=arg=="--render"?output:mapFile;
                if(!path.empty())throw std::runtime_error("Repeated preview option");
                path=argv[++i];if(arg=="--render")render=true;
            } else {std::fprintf(stderr,"Usage: r15-map-preview [--map-file Kolkata.r15map] [--render directory]\n");return 2;}
        }
        const bool realMap=!mapFile.empty();
        const auto scene=realMap?host::loadMapFile(mapFile):host::mapDemoScene();
        if(SDL_Init(SDL_INIT_VIDEO|SDL_INIT_TIMER))throw std::runtime_error(SDL_GetError());
        struct SdlExit {~SdlExit(){SDL_Quit();}} sdlExit;
        lv_init();lv_log_register_print_cb(logMessage);
        auto* display=lv_display_create(800,480);lv_display_set_color_format(display,LV_COLOR_FORMAT_RGB565);
        std::array<std::uint16_t,800*40> buffer{};
        lv_display_set_buffers(display,buffer.data(),nullptr,buffer.size()*2,LV_DISPLAY_RENDER_MODE_PARTIAL);lv_display_set_flush_cb(display,flush);
        {
            MapScreen screen(realMap);ViewOptions view;Lifecycle life;life.key(true,0);life.advance(2000);
            const auto frameAt=[&](std::uint64_t time) {
                if(!realMap)return host::mapDemoFrame(scene,time);
                host::MapDemoFrame frame;frame.pose.point={0,0};
                if(time==46000)frame.pose.point={450,350}; // Explicit camera pan, not movement/GPS.
                return frame;
            };
            if(render) {
                std::filesystem::create_directories(output);
                for(bool track:{false,true})for(bool light:{false,true}) {
                    view.track=track;view.light=light;
                    const auto prefix=std::string(track?"track":"street")+(light?"-day":"-night");
                    const auto frame=frameAt(12000);
                    const auto snapshot=[&](const char* name,bool overview,bool stale,bool missing,bool warning,std::uint64_t time) {
                        screen.update(scene,frameAt(time),view,life,2000,overview,true,stale,missing,warning,1);
                        capture(output/(prefix+"-"+name+".bmp"));
                    };
                    snapshot("follow",false,false,false,false,12000);
                    snapshot("overview",true,false,false,false,12000);
                    snapshot("moved",false,false,false,false,46000);
                    if(!realMap)snapshot("stale",false,true,false,false,12000);
                    snapshot("unavailable",false,false,true,false,12000);
                    snapshot("warnings",false,false,false,true,12000);
                    if(!realMap)snapshot("arrival",true,false,false,false,200000);
                    screen.update(scene,frame,view,life,2000,false,true,false,false,false,3);
                    capture(output/(prefix+"-zoom.bmp"));
                    screen.update(scene,frame,view,life,2000,false,true,false,false,false,1);lv_refr_now(nullptr);
                    const auto before=flushes;
                    screen.update(scene,frame,view,life,2000,false,true,false,false,false,1);lv_refr_now(nullptr);
                    std::printf("Map idle %s: %llu flushes\n",prefix.c_str(),static_cast<unsigned long long>(flushes-before));
                }
                Lifecycle boot;boot.key(true,0);
                screen.update(scene,frameAt(0),view,boot,900,false,true,false,false,false,1);capture(output/"map-boot.bmp");
                screen.update(scene,frameAt(0),view,boot,900,false,true,false,false,true,1);capture(output/"map-boot-warning.bmp");
                Lifecycle shutdown;shutdown.key(true,0);shutdown.advance(2000);shutdown.key(false,3000);
                screen.update(scene,frameAt(0),view,shutdown,3500,false,true,false,false,false,1);capture(output/"map-shutdown.bmp");
                screen.update(scene,frameAt(0),view,shutdown,3500,false,true,false,false,true,1);capture(output/"map-shutdown-warning.bmp");
                std::ofstream metrics(output/"map-metrics.txt");metrics<<(realMap?"Real OSM Kolkata snapshot; no GPS/route; not hardware evidence":"Synthetic scene; not hardware evidence")<<"\nroads="<<scene.roadCount<<"\nroute_points="<<scene.routeCount<<"\nscene_bytes="<<sizeof(scene)<<"\nlvgl_warnings="<<warnings<<"\n";
                std::printf("Rendered 800x480 %s map scenarios to %s; LVGL warnings=%u\n",realMap?"Kolkata OSM":"synthetic",output.string().c_str(),warnings);
            } else {
                using Window=std::unique_ptr<SDL_Window,decltype(&SDL_DestroyWindow)>;
                using Renderer=std::unique_ptr<SDL_Renderer,decltype(&SDL_DestroyRenderer)>;
                using Texture=std::unique_ptr<SDL_Texture,decltype(&SDL_DestroyTexture)>;
                Window window(SDL_CreateWindow(realMap?"R15 LVGL · KOLKATA OSM · NO GPS":"R15 LVGL Map · SYNTHETIC PREVIEW",SDL_WINDOWPOS_CENTERED,SDL_WINDOWPOS_CENTERED,800,480,SDL_WINDOW_ALLOW_HIGHDPI),SDL_DestroyWindow);
                if(!window)throw std::runtime_error(SDL_GetError());
                Renderer renderer(SDL_CreateRenderer(window.get(),-1,SDL_RENDERER_ACCELERATED|SDL_RENDERER_PRESENTVSYNC),SDL_DestroyRenderer);
                if(!renderer)renderer.reset(SDL_CreateRenderer(window.get(),-1,SDL_RENDERER_SOFTWARE));
                if(!renderer)throw std::runtime_error(SDL_GetError());SDL_RenderSetLogicalSize(renderer.get(),800,480);
                SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY,"linear");
                Texture texture(SDL_CreateTexture(renderer.get(),SDL_PIXELFORMAT_RGB565,SDL_TEXTUREACCESS_STREAMING,800,480),SDL_DestroyTexture);
                if(!texture)throw std::runtime_error(SDL_GetError());
                if(realMap)std::puts("KOLKATA OSM snapshot: Arrows pan | R recenter | V area overview | +/- zoom | D theme | Tab layout | U unavailable | W sample warnings | O ignition | Esc quit. No route or GPS; OSM data under ODbL.");
                else std::puts("SYNTHETIC map: D theme | Tab layout | Space pause | V overview/follow | +/- zoom | X stale | U unavailable | W warnings | R restart | O ignition | Esc quit. No GPS/provider/vehicle.");
                const auto start=std::chrono::steady_clock::now();bool running=true,paused=false,overview=false,stale=false,missing=false,warning=false,repaint=true;unsigned zoom=1;
                std::uint64_t previous=0,travel=0;maps::Point camera{};life=Lifecycle{};life.key(true,0);
                while(running) {
                    const auto now=std::uint64_t(std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now()-start).count());
                    const auto delta=now-previous;lv_tick_inc(std::uint32_t(delta));previous=now;
                    SDL_Event e;while(SDL_PollEvent(&e)) {
                        if(e.type==SDL_QUIT)running=false;
                        if(e.type==SDL_WINDOWEVENT)repaint=true;
                        if(e.type!=SDL_KEYDOWN||e.key.repeat)continue;
                        switch(e.key.keysym.sym) {
                        case SDLK_ESCAPE:running=false;break;case SDLK_d:view.light=!view.light;break;
                        case SDLK_TAB:view.track=!view.track;break;case SDLK_SPACE:paused=!paused;break;
                        case SDLK_v:overview=!overview;break;case SDLK_x:if(!realMap)stale=!stale;break;case SDLK_u:missing=!missing;break;
                        case SDLK_w:warning=!warning;break;case SDLK_r:travel=0;camera={};break;case SDLK_o:life.key(!life.ignition(),now);break;
                        case SDLK_LEFT:if(realMap){camera.east=std::max(-1800,camera.east-100);overview=false;}break;
                        case SDLK_RIGHT:if(realMap){camera.east=std::min(1800,camera.east+100);overview=false;}break;
                        case SDLK_UP:if(realMap){camera.north=std::min(2000,camera.north+100);overview=false;}break;
                        case SDLK_DOWN:if(realMap){camera.north=std::max(-2000,camera.north-100);overview=false;}break;
                        case SDLK_EQUALS:case SDLK_PLUS:zoom=std::min(3u,zoom+1);break;case SDLK_MINUS:zoom=std::max(1u,zoom-1);break;
                        default:break;
                        }
                    }
                    life.advance(now);
                    if(!realMap&&!paused&&!stale&&!missing&&life.phase()==Phase::Running)travel+=delta;
                    auto frame=frameAt(travel);if(realMap)frame.pose.point=camera;
                    screen.update(scene,frame,view,life,now,overview,paused,stale,missing,warning,zoom);lv_timer_handler();
                    if(dirty){if(SDL_UpdateTexture(texture.get(),nullptr,pixels.data(),1600))throw std::runtime_error(SDL_GetError());dirty=false;repaint=true;}
                    if(repaint){SDL_RenderClear(renderer.get());SDL_RenderCopy(renderer.get(),texture.get(),nullptr,nullptr);SDL_RenderPresent(renderer.get());repaint=false;}
                    std::this_thread::sleep_for(std::chrono::milliseconds(20));
                }
            }
        }
        lv_display_delete(display);lv_deinit();return warnings?1:0;
    } catch(const std::exception& e){std::fprintf(stderr,"Map preview failed: %s\n",e.what());return 1;}
}
