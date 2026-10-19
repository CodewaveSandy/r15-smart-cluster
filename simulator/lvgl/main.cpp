#include "embedded/ui/Dashboard.h"
#include "embedded/ui/Typography.h"
#include <SDL.h>
#include <algorithm>
#include <array>
#include <chrono>
#include <cstdio>
#include <filesystem>
#include <memory>
#include <stdexcept>
#include <string>
#include <thread>
#include <ctime>
#include <vector>
#include <fstream>

namespace {
using namespace cluster;
std::array<std::uint16_t,800*480> pixels{};
int warnings=0;
struct FlushMetrics { std::uint64_t calls=0, pixels=0; };
FlushMetrics flushMetrics;
void logMessage(lv_log_level_t, const char* message) { ++warnings; std::fprintf(stderr,"%s",message); }
void flush(lv_display_t* display,const lv_area_t* area,std::uint8_t* bytes) {
    ++flushMetrics.calls;
    flushMetrics.pixels+=std::uint64_t(area->x2-area->x1+1)*(area->y2-area->y1+1);
    const auto* source=reinterpret_cast<std::uint16_t*>(bytes);
    for(int y=area->y1;y<=area->y2;++y)
        std::copy_n(source+(y-area->y1)*(area->x2-area->x1+1),area->x2-area->x1+1,pixels.data()+y*800+area->x1);
    lv_display_flush_ready(display);
}
ClusterState demo() {
    ClusterState s;
    s.bestLapMs={78420,Quality::Available}; // Static best lap sample: 01:18.42.
    s.currentLapMs={84380,Quality::Available}; // Static lap sample: 01:24.38.
    s.speedKph={60,Quality::Available}; s.engineRpm={8500,Quality::Available};
    s.quickShifter={true,Quality::Available}; s.vva={true,Quality::Available};
    s.phoneConnected={true,Quality::Available}; s.phoneBatteryPercent={84,Quality::Available};
    s.gear={4,Quality::Available}; s.fuelPercent={72,Quality::Available};
    s.odometerKm={12480,Quality::Available};
    // Static host preview samples, not computed or persisted vehicle distances.
    s.tripAKm={128.4,Quality::Available}; s.tripBKm={46.8,Quality::Available}; s.tripFKm={0,Quality::Available};
    s.tcsEnabled={true,Quality::Available}; s.batteryVoltageV={12.8,Quality::Available};
    s.coolantTemperatureC={84,Quality::Available};
    s.left=s.right=s.highBeam=s.absWarning=s.engineWarning=s.coolantWarning={false,Quality::Available};
    return s;
}
void capture(const std::filesystem::path& path) {
    lv_refr_now(nullptr);
    auto* surface=SDL_CreateRGBSurfaceWithFormatFrom(pixels.data(),800,480,16,800*2,SDL_PIXELFORMAT_RGB565);
    if(!surface) throw std::runtime_error(SDL_GetError());
    const int result=SDL_SaveBMP(surface,path.string().c_str()); SDL_FreeSurface(surface);
    if(result) throw std::runtime_error(SDL_GetError());
}
void help() {
    std::puts("R15 LVGL preview — simulated data only\n"
        "Left/Right: speed +/-1 | Up/Down: RPM +/-100 | [ / ]: fuel +/-1\n"
        "0..6: gear (0 neutral) | Tab: Street/Track | D: Midnight/Daylight\n"
        "Enter: Street trip page | I: info page | T: toggle selected TCS/Track (simulation only)\n"
        "U: metric/imperial | X: unavailable/restore\n"
        "L/R: turn signals | Z: hazards | H: high beam | A/E/C: warnings\n"
        "O: key on/off (1250 ms animation) | Space: reset demo\n"
        "F1: print controls | F2: all icons | Esc: quit | No real bike connected.");
}
}
int main(int argc,char** argv) {
    try {
        bool render=false; std::filesystem::path output;
        if(argc==3&&std::string(argv[1])=="--render") { render=true; output=argv[2]; }
        else if(argc!=1) { std::fprintf(stderr,"Usage: r15-preview [--render directory]\n"); return 2; }
        if(SDL_Init(SDL_INIT_VIDEO|SDL_INIT_TIMER)) throw std::runtime_error(SDL_GetError());
        struct SdlExit { ~SdlExit(){SDL_Quit();} } sdlExit;
        lv_init();
        lv_log_register_print_cb(logMessage);
        auto* display=lv_display_create(800,480);
        lv_display_set_color_format(display,LV_COLOR_FORMAT_RGB565);
        std::vector<std::uint16_t> drawBuffer(800*40);
        lv_display_set_buffers(display,drawBuffer.data(),nullptr,drawBuffer.size()*2,LV_DISPLAY_RENDER_MODE_PARTIAL);
        lv_display_set_flush_cb(display,flush);
        {
            cluster::ui::Dashboard dashboard(lv_screen_active());
            ClusterState state=demo(); cluster::ui::ViewOptions view; Lifecycle life;
            if(render) {
                std::filesystem::create_directories(output);
                view.time="20:55"; view.date="18 Oct 2026";
                life.key(true,0); life.advance(1250);
                for(bool track:{false,true}) for(bool light:{false,true}) {
                    view.track=track; view.light=light;
                    const std::string suffix=std::string(track?"track":"street")+(light?"-day":"-night");
                    dashboard.update(state,view,life,1400); capture(output/(suffix+".bmp"));
                    const auto palette=light?cluster::ui::daylight:cluster::ui::midnight;
                    if(pixels[cluster::ui::Metrics::lowerDividerY*800+300]!=lv_color_to_u16(lv_color_hex(palette.line)) ||
                        pixels[(cluster::ui::Metrics::rpmY+cluster::ui::Metrics::rpmHeight)*800+300]!=lv_color_to_u16(lv_color_hex(palette.background)))
                        throw std::runtime_error("Layout vertical geometry differs from shared metrics");
                    auto low=state; low.fuelPercent.value=9;
                    dashboard.update(low,view,life,0); capture(output/(suffix+"-fuel-on.bmp"));
                    const auto fuelOn=pixels[cluster::ui::Metrics::fuelY*800+590];
                    dashboard.update(low,view,life,500); capture(output/(suffix+"-fuel-off.bmp"));
                    if(fuelOn==pixels[cluster::ui::Metrics::fuelY*800+590]) throw std::runtime_error("Fuel blink did not reach rendered pixels");
                    ClusterState missing; dashboard.update(missing,view,life,1400); capture(output/(suffix+"-unavailable.bmp"));
                    auto warning=state; warning.engineRpm.value=11400; warning.engineWarning.value=true; warning.absWarning.value=true;
                    dashboard.update(warning,view,life,1200); capture(output/(suffix+"-warning.bmp"));
                }
                Lifecycle boot; boot.key(true,0); dashboard.update(state,view,boot,625); capture(output/"boot.bmp");
                boot.key(false,700); dashboard.update(state,view,boot,1325); capture(output/"shutdown.bmp");
                auto urgentState=state; urgentState.engineWarning.value=true;
                dashboard.update(urgentState,view,boot,1325); capture(output/"shutdown-warning.bmp");
                Lifecycle off; dashboard.update(ClusterState{},view,off,0); capture(output/"off.bmp");
                for(bool track:{false,true}) for(bool light:{false,true}) {
                    view.track=track; view.light=light; view.iconTest=true;
                    const auto name=std::string("icons-")+(track?"track":"street")+(light?"-day.bmp":"-night.bmp");
                    dashboard.update(state,view,life,1400); capture(output/name);
                }
                view.iconTest=false;
                for(bool track:{false,true}) for(bool light:{false,true}) {
                    view.track=track; view.light=light;
                    view.tripPage=TripPage::Odometer; view.infoPage=InfoPage::Coolant;
                    std::vector<std::uint16_t> bestPanel;
                    for(int page=0;page<4;++page) {
                        auto sample=state;
                        if(page==2) sample.tcsEnabled.value=false;
                        dashboard.update(sample,view,life,1400);
                        const auto name=std::string(track?"track":"street")+(light?"-day":"-night")+"-pages-"+std::to_string(page)+".bmp";
                        capture(output/name);
                        if(track) {
                            std::vector<std::uint16_t> panel;
                            for(int y=399;y<460;++y)
                                panel.insert(panel.end(),pixels.begin()+y*800+28,pixels.begin()+y*800+248);
                            if(page==0) bestPanel=panel;
                            else if(panel!=bestPanel) throw std::runtime_error("Track best-lap panel changed with Street trip selection");
                        }
                        view.tripPage=next(view.tripPage); view.infoPage=next(view.infoPage);
                    }
                }
                // Check the actual rendered digit bounds, not just the label's position.
                view.iconTest=false;
                for(bool track:{false,true}) for(bool light:{false,true}) {
                    view.track=track; view.light=light;
                    const auto palette=light?cluster::ui::daylight:cluster::ui::midnight;
                    const auto target=lv_color_to_u16(lv_color_hex(palette.ink));
                    for(int value:{-1,0,1,60,180}) {
                        auto sample=state;
                        sample.speedKph=value<0?Signal<int>{}:Signal<int>{value,Quality::Available};
                        if(track) sample.currentLapMs=value<0?Signal<std::uint32_t>{}:Signal<std::uint32_t>{static_cast<std::uint32_t>(value)*1000,Quality::Available};
                        dashboard.update(sample,view,life,1400); lv_refr_now(nullptr);
                        int left=800,right=-1;
                        const auto* font=track?cluster::ui::fonts::track:cluster::ui::fonts::speed;
                        const int primaryY=track?cluster::ui::Metrics::trackLapY:240;
                        for(int y=primaryY;y<primaryY+font->line_height;++y)
                            for(int x=cluster::ui::Metrics::speedX;x<cluster::ui::Metrics::speedX+cluster::ui::Metrics::speedWidth;++x)
                                if(pixels[y*800+x]==target) { left=std::min(left,x); right=std::max(right,x); }
                        if(right<left||std::abs(left+right-799)>4) throw std::runtime_error("Primary instrument glyphs are not centered on the viewport");
                        if(track) {
                            left=800; right=-1;
                            for(int y=269;y<269+cluster::ui::fonts::secondary->line_height;++y)
                                for(int x=560;x<772;++x)
                                    if(pixels[y*800+x]==target) { left=std::min(left,x); right=std::max(right,x); }
                            if(right<left||std::abs(left+right-1331)>4) throw std::runtime_error("Track speed is not centered in its secondary position");
                        }
                    }
                }
                // Host work counters, not ESP32 timing or memory measurements.
                view.track=false; view.light=false;
                dashboard.update(state,view,life,1400); lv_refr_now(nullptr);
                const auto stablePixels=pixels;
                flushMetrics={};
                const auto measuredStart=std::chrono::steady_clock::now();
                for(int i=0;i<100;++i) {
                    dashboard.update(state,view,life,1400);
                    lv_refr_now(nullptr);
                }
                if(flushMetrics.calls!=0) throw std::runtime_error("Identical dashboard updates caused redundant flushes");
                const auto idleMetrics=flushMetrics;
                const auto elapsedUs=std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now()-measuredStart).count();
                if(pixels!=stablePixels) throw std::runtime_error("Repeated identical presentation changed pixels");
                const auto countObjects=[](auto&& self,lv_obj_t* obj)->std::uint32_t {
                    std::uint32_t count=1;
                    for(std::uint32_t i=0;i<lv_obj_get_child_count(obj);++i) count+=self(self,lv_obj_get_child(obj,i));
                    return count;
                };
                std::ofstream report(output/"host-metrics.json");
                report << "{\n  \"scope\": \"host only; not firmware resource or timing evidence\",\n"
                    << "  \"scenario\": \"100 identical Street/Midnight updates at fixed monotonic time\",\n"
                    << "  \"iterations\": 100,\n  \"flush_calls\": " << idleMetrics.calls
                    << ",\n  \"flushed_pixels\": " << idleMetrics.pixels
                    << ",\n  \"elapsed_us\": " << elapsedUs
                    << ",\n  \"lvgl_object_count_including_screen\": " << countObjects(countObjects,lv_screen_active())
                    << ",\n  \"dashboard_wrapper_bytes_on_host\": " << sizeof(cluster::ui::Dashboard)
                    << ",\n  \"host_framebuffer_bytes\": " << pixels.size()*sizeof(pixels[0])
                    << ",\n  \"host_draw_buffer_bytes\": " << drawBuffer.size()*sizeof(drawBuffer[0]) << "\n}\n";
                if(!report) throw std::runtime_error("Cannot write host metrics report");
                // Include shift palettes, unknown observations, icon tests, and covered artwork.
                // Blink tests elsewhere require changes to reach pixels; these checks fix time.
                for(bool track:{false,true}) for(bool light:{false,true}) {
                    view.track=track; view.light=light;
                    for(int scenario=0;scenario<6;++scenario) {
                        auto sample=state; Lifecycle sceneLife=life;
                        view.iconTest=scenario==3;
                        if(scenario==1) sample.engineRpm.value=11400;
                        if(scenario==2) sample={};
                        if(scenario==4) { sample.fuelPercent.value=9; sample.left.value=true; }
                        if(scenario==5) { sceneLife=Lifecycle{}; sceneLife.key(true,0); }
                        dashboard.update(sample,view,sceneLife,600); lv_refr_now(nullptr);
                        const auto expected=pixels;
                        flushMetrics={};
                        for(int i=0;i<3;++i) { dashboard.update(sample,view,sceneLife,600); lv_refr_now(nullptr); }
                        if(flushMetrics.calls || pixels!=expected) {
                            std::fprintf(stderr,"Stable scenario: track=%d light=%d scenario=%d flushes=%llu\n",track,light,scenario,static_cast<unsigned long long>(flushMetrics.calls));
                            throw std::runtime_error("Stable layout/theme/warning/lifecycle scenario redrew or changed pixels");
                        }
                    }
                }
                // Parent-owned LVGL objects must disappear when their Dashboard owner dies.
                const auto retainedObjects=countObjects(countObjects,lv_screen_active());
                const auto retainedPixels=pixels;
                for(int cycle=0;cycle<8;++cycle) {
                    {
                        cluster::ui::Dashboard temporary(lv_screen_active());
                        if(countObjects(countObjects,lv_screen_active())!=retainedObjects*2-1)
                            throw std::runtime_error("Dashboard component construction changed object budget");
                        cluster::ui::ViewOptions temporaryView;
                        temporaryView.track=cycle%2; temporaryView.light=(cycle/2)%2;
                        temporary.update(state,temporaryView,life,1400);
                        lv_refr_now(nullptr);
                    }
                    if(countObjects(countObjects,lv_screen_active())!=retainedObjects)
                        throw std::runtime_error("Dashboard destruction leaked LVGL objects");
                    lv_refr_now(nullptr);
                    if(pixels!=retainedPixels)
                        throw std::runtime_error("Dashboard destruction did not restore underlying screen");
                }
                if(warnings) throw std::runtime_error("LVGL render diagnostics reported warnings");
                std::printf("Rendered 44 scenarios to %s\n",output.string().c_str());
            } else {
                using Window=std::unique_ptr<SDL_Window,decltype(&SDL_DestroyWindow)>;
                using Renderer=std::unique_ptr<SDL_Renderer,decltype(&SDL_DestroyRenderer)>;
                using Texture=std::unique_ptr<SDL_Texture,decltype(&SDL_DestroyTexture)>;
                Window window(SDL_CreateWindow("R15 Smart Cluster · LVGL · Simulated",SDL_WINDOWPOS_CENTERED,SDL_WINDOWPOS_CENTERED,800,480,SDL_WINDOW_ALLOW_HIGHDPI),SDL_DestroyWindow);
                if(!window) throw std::runtime_error(SDL_GetError());
                Renderer renderer(SDL_CreateRenderer(window.get(),-1,SDL_RENDERER_ACCELERATED|SDL_RENDERER_PRESENTVSYNC),SDL_DestroyRenderer);
                if(!renderer) renderer.reset(SDL_CreateRenderer(window.get(),-1,SDL_RENDERER_SOFTWARE));
                if(!renderer) throw std::runtime_error(SDL_GetError());
                SDL_RenderSetLogicalSize(renderer.get(),800,480);
                // Host-only interpolation for Retina enlargement; LVGL still renders 800x480.
                SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY,"linear");
                Texture texture(SDL_CreateTexture(renderer.get(),SDL_PIXELFORMAT_RGB565,SDL_TEXTUREACCESS_STREAMING,800,480),SDL_DestroyTexture);
                if(!texture) throw std::runtime_error(SDL_GetError());
                const auto start=std::chrono::steady_clock::now();
                auto milliseconds=[&]{return std::uint64_t(std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now()-start).count());};
                life.key(true,0); bool running=true,missing=false; help(); std::uint64_t last=0;
                while(running) {
                    const auto now=milliseconds(); SDL_Event event;
                    while(SDL_PollEvent(&event)) {
                        if(event.type==SDL_QUIT) running=false;
                        if(event.type!=SDL_KEYDOWN) continue;
                        const auto key=event.key.keysym.sym;
                        if(event.key.repeat && key!=SDLK_LEFT&&key!=SDLK_RIGHT&&key!=SDLK_UP&&key!=SDLK_DOWN&&key!=SDLK_LEFTBRACKET&&key!=SDLK_RIGHTBRACKET) continue;
                        switch(key) {
                        case SDLK_ESCAPE: running=false; break;
                        case SDLK_RIGHT: state.speedKph.value=std::min(180,state.speedKph.value+1); break;
                        case SDLK_LEFT: state.speedKph.value=std::max(0,state.speedKph.value-1); break;
                        case SDLK_UP: state.engineRpm.value=std::min(13000,state.engineRpm.value+100); break;
                        case SDLK_DOWN: state.engineRpm.value=std::max(0,state.engineRpm.value-100); break;
                        case SDLK_LEFTBRACKET: state.fuelPercent.value=std::max(0,state.fuelPercent.value-1); break;
                        case SDLK_RIGHTBRACKET: state.fuelPercent.value=std::min(100,state.fuelPercent.value+1); break;
                        case SDLK_TAB: view.track=!view.track; break;
                        case SDLK_d: view.light=!view.light; break;
                        case SDLK_u: view.imperial=!view.imperial; break;
                        case SDLK_RETURN: if(!view.track) view.tripPage=next(view.tripPage); break;
                        case SDLK_i: view.infoPage=next(view.infoPage); break;
                        case SDLK_t:
                            if(view.infoPage==InfoPage::Track) view.track=!view.track;
                            else if(view.infoPage==InfoPage::Tcs&&!missing&&state.tcsEnabled.available()) state.tcsEnabled.value=!state.tcsEnabled.value;
                            break;
                        case SDLK_x: missing=!missing; break;
                        case SDLK_o: life.key(!life.ignition(),now); break;
                        case SDLK_l: state.left.value=!state.left.value; state.right.value=false; break;
                        case SDLK_r: state.right.value=!state.right.value; state.left.value=false; break;
                        case SDLK_z: { const bool on=!(state.left.value&&state.right.value); state.left.value=state.right.value=on; break; }
                        case SDLK_h: state.highBeam.value=!state.highBeam.value; break;
                        case SDLK_a: state.absWarning.value=!state.absWarning.value; break;
                        case SDLK_e: state.engineWarning.value=!state.engineWarning.value; break;
                        case SDLK_c: state.coolantWarning.value=!state.coolantWarning.value; break;
                        case SDLK_SPACE: state=demo(); missing=false; break;
                        case SDLK_F1: help(); break;
                        case SDLK_F2: view.iconTest=!view.iconTest; break;
                        default: if(key>=SDLK_0&&key<=SDLK_6) state.gear.value=key-SDLK_0; break;
                        }
                    }
                    life.advance(now);
                    state.vva.value=state.engineRpm.value>7500; // Simulator-only display demonstration.
                    const auto wall=std::time(nullptr); std::tm local{}; localtime_r(&wall,&local);
                    char timeText[16],dateText[32]; std::strftime(timeText,sizeof timeText,"%H:%M",&local); std::strftime(dateText,sizeof dateText,"%d %b %Y",&local);
                    view.time=timeText; view.date=dateText;
                    lv_tick_inc(static_cast<std::uint32_t>(now-last)); last=now;
                    const auto observed=missing?ClusterState{}:state;
                    if(!valid(observed)) throw std::runtime_error("Simulator produced invalid normalized state");
                    dashboard.update(observed,view,life,now); lv_timer_handler();
                    SDL_UpdateTexture(texture.get(),nullptr,pixels.data(),800*2);
                    SDL_RenderClear(renderer.get()); SDL_RenderCopy(renderer.get(),texture.get(),nullptr,nullptr); SDL_RenderPresent(renderer.get());
                    std::this_thread::sleep_for(std::chrono::milliseconds(8));
                }
            }
        }
        lv_display_delete(display); lv_deinit(); return 0;
    } catch(const std::exception& e) { std::fprintf(stderr,"Preview failed: %s\n",e.what()); return 1; }
}
